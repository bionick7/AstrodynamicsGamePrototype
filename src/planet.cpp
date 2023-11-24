#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"

static std::map<std::string, PlanetNature> ephemerides = std::map<std::string, PlanetNature>();
static PlanetNature parent = {0};

Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;
    orbit = OrbitFromElements(1, 0, 0, 1, 0, true);
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = 0;
        resource_delta[i] = 0;
        resource_capacity[i] = 1e7;
    }
}

void Planet::Serialize(DataNode* data) const {
    data->Set("name", name);
    //data->SetF("mass", mu / G);
    //data->SetF("radius", radius);

    DataNode* resource_node = data->SetChild("resource_stock", DataNode());
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        resource_node->SetF(resources_names[resource_index], resource_stock[resource_index] / 1000);
    }
    int last_building_index = 0;
    for(int i=0; i < MAX_PLANET_BUILDINGS; i++) 
        if (buildings[i].IsValid()) last_building_index = i;

    data->SetArray("buildings", last_building_index);
    for(int i=0; i < last_building_index; i++) {
        const BuildingClass* mc = GetBuildingByIndex(buildings[i].class_index);
        data->SetArrayElem("buildings", i, mc->id);
    }

    // We assume the orbital info is stored in the ephemerides
}

void Planet::Deserialize(const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    auto find = ephemerides.find(name);
    if (find == ephemerides.end()) {
        ERROR("Could not find planet %s in ephemerides", name)
        return;
    }
    //*this = *((Planet*) (void*) &find->second);
    PlanetNature nature = find->second;
    mu = nature.mu;
    radius = nature.radius;
    orbit = nature.orbit;

    const DataNode* resource_node = data->GetChild("resource_stock", true);
    if (resource_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            resource_stock[resource_index] = resource_node->GetF(resources_names[resource_index], 0, true) * 1000;
        }
    }

    if (data->Has("buildings")) {
        int initial_building_count = data->GetArrayLen("buildings", true);
        if (initial_building_count > MAX_PLANET_BUILDINGS) {
            initial_building_count = MAX_PLANET_BUILDINGS;
        }
        
        for (int i = 0; i < initial_building_count; i++) {
            const char* building_id = data->GetArray("buildings", i);
            buildings[i] = BuildingInstance(GetBuildingIndexById(building_id));
        }
    }
}

void Planet::_OnClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (
        tp_ui.plan != NULL
        && IsIdValid(tp_ui.ship)
        && IsIdValid(tp_ui.plan->departure_planet)
        && !IsIdValid(tp_ui.plan->arrival_planet)
    ) {
        tp_ui.SetDestination(id);
    } else if (GlobalGetState()->focused_planet == id) {
        GetScreenTransform()->focus = position.cartesian;
    } else {
        GlobalGetState()->focused_planet = id;
    }
}

double Planet::ScreenRadius() const {
    return fmax(GetScreenTransform()->TransformS(radius), 4);
}

double Planet::GetDVFromExcessVelocity(Vector2 vel) const {
    return sqrt(mu / (2*radius) + Vector2LengthSqr(vel));
}

resource_count_t Planet::DrawResource(int resource_id, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to give
    if (resource_id < 0 || resource_id >= RESOURCE_MAX) {
        return 0;
    }
    
    resource_count_t transferred_resources = ClampInt(quantity, 0, resource_stock[resource_id]);
    resource_stock[resource_id] -= transferred_resources;

    return transferred_resources;
}

resource_count_t Planet::GiveResource(int resource, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to take
    if (resource < 0 || resource >= RESOURCE_MAX) {
        return 0;
    }

    resource_count_t transferred_resources = Clamp(quantity, 0, resource_capacity[resource]);
    resource_stock[resource] += transferred_resources;

    return transferred_resources;
}

void Planet::RecalcStats() {
    // Just call this every frame tbh
    for (int i = 0; i < RESOURCE_MAX; i++){
        resource_delta[i] = 0;
    }
    
    for (int i = 0; i < STAT_MAX; i++){
        stats[i] = 0;
    }

    for (int i = 0; i < MAX_PLANET_BUILDINGS; i++) {
        if (buildings[i].IsValid()) {
            buildings[i].Effect(&resource_delta[0], &stats[0]);
        }
    }
}

void Planet::RequestBuild(int slot, building_index_t building_class) {
    const BuildingClass* mc = GetBuildingByIndex(building_class);
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        if (mc->build_costs[resource_index] > resource_stock[resource_index]) {
            PLAYER_INFO("Not enough %s (%f available, %f required)", 
                resources_names[resource_index],
                resource_stock [resource_index],
                mc->build_costs[resource_index]
            )
            return;
        }
    }
    BuildingInstance instance = BuildingInstance(building_class);
    buildings[slot] = instance;

    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        resource_stock[resource_index] -= mc->build_costs[resource_index];
    }
}

bool Planet::HasMouseHover(double* min_distance) const {
    Vector2 screen_pos = GetScreenTransform()->TransformV(position.cartesian);
    double dist = Vector2Distance(GetMousePosition(), screen_pos);
    if (dist <= ScreenRadius() * 1.2 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}

void Planet::Update() {
    Time now = GlobalGetNow();
    Time prev = GlobalGetPreviousFrameTime();
    RecalcStats();
    position = OrbitGetPosition(&orbit, now);
    double delta_T = TimeDays(TimeSub(now, prev));
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = Clamp(resource_stock[i] + resource_delta[i] * delta_T, 0, resource_capacity[i]);
    }
}

void Planet::Draw(const CoordinateTransform* c_transf) {
    //printf("%f : %f\n", position.x, position.y);
    Vector2 screen_pos = c_transf->TransformV(position.cartesian);
    DrawCircleV(screen_pos, ScreenRadius(), MAIN_UI_COLOR);
    
    DrawOrbit(&orbit, MAIN_UI_COLOR);

    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    int text_h = 16;
    Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), name, text_h, 1);
    DrawTextEx(GetCustomDefaultFont(), name, {screen_x - text_size.x / 2,  screen_y - text_size.y - 5}, text_h, 1, MAIN_UI_COLOR);

    if (mouse_hover) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 10, TRANSFER_UI_COLOR);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }
}

void _UIDrawResources(
    const resource_count_t resource_stock[], const resource_count_t resource_delta[], const resource_count_t resource_cap[], 
    const ResourceTransfer& transfer, double fuel_draw
) {
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", resources_names[i], qtt, cap, delta);
        sprintf(buffer, "%-10s %3.1fT (%+3d T/d)", resources_names[i], resource_stock[i] / 1e3, (int)(resource_delta[i]/1e3));
        UIContextPushInset(0, 18);
            if (GlobalGetState()->active_transfer_plan.IsActive()) {
                // Button
                if (UIContextDirectButton(transfer.resource_id == i ? "X" : " ", 2) & BUTTON_STATE_FLAG_JUST_PRESSED) {
                    GlobalGetState()->active_transfer_plan.SetResourceType(i);
                }
            }
            UIContextWrite(buffer, false);

            double qtt = 0;
            if (transfer.resource_id == i) {
                qtt += transfer.quantity;
            }
            if (fuel_draw > 0 && i == RESOURCE_WATER) {
                qtt -= fuel_draw;
            }
            if (qtt != 0) {
                sprintf(buffer, "   %+3.1fK", qtt / 1000);
                UIContextWrite(buffer, false);
            }
            UIContextFillline(resource_stock[i] / resource_cap[i], MAIN_UI_COLOR, BG_COLOR);
        UIContextPop();  // Inset
        //TextBoxLineBreak(&tb);
    }
}

void _UIDrawStats(const resource_count_t stats[]) {
    for (int i=0; i < STAT_MAX; i++) {
        char buffer[50];
        sprintf(buffer, "%-10s %3.1f", stat_names[i], stats[i]);
        UIContextWrite(buffer);
        //TextBoxLineBreak(&tb);
    }
}

void Planet::DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer, double fuel_draw) {
    if (upper_quadrant) {
        UIContextCreate(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    } else {
        UIContextCreate(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    }
    UIContextCurrent().Enclose(2, 2, BG_COLOR, MAIN_UI_COLOR);

    UIContextWrite(name);
    UIContextWrite("-------------------------");
    _UIDrawResources(resource_stock, resource_delta, resource_capacity, transfer, fuel_draw);
    UIContextWrite("-------------------------");
    _UIDrawStats(stats);
    UIContextWrite("-------------------------");

    // Draw buildings
    int current_width = UIContextCurrent().width;
    UIContextPushInset(0, UIContextCurrent().height - UIContextCurrent().y_cursor);
    UIContextPushHSplit(0, current_width/2);
    for (int i = 0; i < MAX_PLANET_BUILDINGS; i += 2) {
        if (buildings[i].UIDraw()) {
            BuildingConstructionOpen(id, i);
        }
    }
    UIContextPop();  // HSplit
    UIContextPushHSplit(current_width/2, current_width);
    for (int i = 1; i < MAX_PLANET_BUILDINGS; i += 2) {
        if (buildings[i].UIDraw()) {
            BuildingConstructionOpen(id, i);
        }
    }
    UIContextPop();  // HSplit
    UIContextPop();  // Inset
}

const PlanetNature* GetParentNature() {
    return &parent;
}

int LoadEphemerides(const DataNode* data) {
    // Init planets
    parent.radius = data->GetF("radius");
    parent.mu = data->GetF("mass") * G;
    int num_planets = data->GetArrayChildLen("satellites");
    //if (num_planets > 100) num_planets = 100;
    //entity_id_t planets[100];
    for(int i=0; i < num_planets; i++) {
        const DataNode* planet_data = data->GetArrayChild("satellites", i);
        const char* name = planet_data->Get("name");
        ephemerides.insert_or_assign(name, PlanetNature());
        PlanetNature* nature = &ephemerides.at(name);
        
        double sma = planet_data->GetF("SMA");
        double ann = planet_data->GetF("Ann") * DEG2RAD;
        Time epoch = TimeSub(GlobalGetNow(), Time(ann / sqrt(parent.mu / (sma*sma*sma))));

        nature->mu = planet_data->GetF("mass") * G;
        nature->radius = planet_data->GetF("radius");
        nature->orbit = OrbitFromElements(
            sma,
            planet_data->GetF("Ecc"),
            (planet_data->GetF("LoA") + planet_data->GetF("AoP")) * DEG2RAD,
            parent.mu,
            epoch, 
            strcmp(planet_data->Get("retrograde", "y", true), "y") != 0
        );
    }
    return num_planets;
}
