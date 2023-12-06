#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"


Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;
    orbit = OrbitFromElements(1, 0, 0, 1, 0, true);

    for (int i = 0; i < MAX_PLANET_BUILDINGS; i++) {
        buildings[i] = BuildingInstance(BUILDING_INDEX_INVALID);
    }
}

void Planet::Serialize(DataNode* data) const {
    data->Set("name", name);
    //data->SetF("mass", mu / G);
    //data->SetF("radius", radius);

    DataNode* resource_node = data->SetChild("resource_stock", DataNode());
    DataNode* resource_delta_node = data->SetChild("resource_delta", DataNode());
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        resource_node->SetF(GetResourceData(resource_index).name, economy.resource_stock[resource_index] / 1000);
        resource_delta_node->SetF(GetResourceData(resource_index).name, economy.resource_delta[resource_index] / 1000);
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

void Planet::Deserialize(Planets* planets, const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    auto find = planets->ephemerides.find(name);
    if (find == planets->ephemerides.end()) {
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
            economy.resource_stock[resource_index] = resource_node->GetF(GetResourceData(resource_index).name, 0, true) * 1000;
        }
    }
    const DataNode* resource_delta_node = data->GetChild("resource_delta", true);
    if (resource_delta_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            economy.resource_delta[resource_index] = resource_delta_node->GetF(GetResourceData(resource_index).name, 0, true) * 1000;
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
    economy.RecalcEconomy();
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

void Planet::RecalcStats() {
    // Just call this every frame tbh
    for (int i = 0; i < RESOURCE_MAX; i++){
        economy.resource_delta[i] = 0;
    }
    
    for (int i = 0; i < STAT_MAX; i++){
        stats[i] = 0;
    }

    for (int i = 0; i < MAX_PLANET_BUILDINGS; i++) {
        if (buildings[i].IsValid()) {
            buildings[i].Effect(&economy.resource_delta[0], &stats[0]);
        }
    }
}

void Planet::RequestBuild(int slot, building_index_t building_class) {
    const BuildingClass* mc = GetBuildingByIndex(building_class);
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        if (mc->build_costs[resource_index] > economy.resource_stock[resource_index]) {
            USER_INFO("Not enough %s (%f available, %f required)", 
                GetResourceData(resource_index).name,
                economy.resource_stock[resource_index],
                mc->build_costs[resource_index]
            )
            return;
        }
    }
    BuildingInstance instance = BuildingInstance(building_class);
    buildings[slot] = instance;

    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        economy.resource_stock[resource_index] -= mc->build_costs[resource_index];
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
    timemath::Time now = GlobalGetNow();
    position = OrbitGetPosition(&orbit, now);
    // RecalcStats();
    economy.Update();
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
        DrawCircleLines(screen_x, screen_y, 20, TRANSFER_UI_COLOR);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
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

void _UIDrawBuildings(Planet* planet) {
    // Draw buildings
    int current_width = UIContextCurrent().width;
    UIContextPushInset(0, UIContextCurrent().height - UIContextCurrent().y_cursor);
    UIContextPushHSplit(0, current_width/2);
    for (int i = 0; i < MAX_PLANET_BUILDINGS; i += 2) {
        if (planet->buildings[i].UIDraw()) {
            BuildingConstructionOpen(planet->id, i);
        }
    }
    UIContextPop();  // HSplit
    UIContextPushHSplit(current_width/2, current_width);
    for (int i = 1; i < MAX_PLANET_BUILDINGS; i += 2) {
        if (planet->buildings[i].UIDraw()) {
            BuildingConstructionOpen(planet->id, i);
        }
    }
    UIContextPop();  // HSplit
}

int current_tab = 0;  // Global variable, I suppose
void Planet::DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer, double fuel_draw) {
    if (upper_quadrant) {
        UIContextCreate(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    } else {
        UIContextCreate(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    }
    UIContextCurrent().Enclose(2, 2, BG_COLOR, MAIN_UI_COLOR);

    UIContextPushInset(4, 20);  // Tab container
    int w = UIContextCurrent().width;
    const int n_tabs = 4;
    const char* tab_descriptions[4] = {
        "resources",
        "economy",
        "buildings",
        "quests"
    };
    for (int i=0; i < n_tabs; i++) {
        UIContextPushHSplit(i * w / n_tabs, (i + 1) * w / n_tabs);
        ButtonStateFlags button_state = UIContextAsButton();
        HandleButtonSound(button_state & BUTTON_STATE_FLAG_JUST_PRESSED);
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            current_tab = i;
        }
        if (button_state & BUTTON_STATE_FLAG_HOVER || i == current_tab) {
            UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
        }
        UIContextWrite(tab_descriptions[i]);
        UIContextPop();  // HSplit
    }
    UIContextPop();  // Tab container

    UIContextWrite(name);
    UIContextFillline(1, MAIN_UI_COLOR, MAIN_UI_COLOR);
    //_UIDrawStats(stats);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(transfer, fuel_draw);
        break;
    case 1:
        economy.UIDrawEconomy(transfer, fuel_draw);
        break;
    case 2:
        _UIDrawBuildings(this);
        break;
    }

    UIContextPop();  // Outside
}

Planets::Planets() {
    planet_array = NULL;
    planet_count = 0;
    planet_array_iter = 0;
}

Planets::~Planets() {
    delete[] planet_array;
}

void Planets::Init(entity_id_t p_planet_count) {
    planet_count = p_planet_count;
    planet_array = new Planet[planet_count];
    parent = {0};
    ephemerides = std::map<std::string, PlanetNature>();
}

entity_id_t Planets::AddPlanet(const DataNode* data) {
    //entity_map.insert({uuid, planet_entity});
    entity_id_t id = planet_array_iter++;
    planet_array[id].Deserialize(this, data);
    planet_array[id].Update();
    planet_array[id].id = id;
    return id;
}

Planet* Planets::GetPlanet(entity_id_t id) const {
    if (id >= planet_count) {
        FAIL("Invalid planet id (%d)", id)
    }
    return &planet_array[(int)id];
}

entity_id_t Planets::GetPlanetCount() const {
    return planet_count;
}

Planet* Planets::GetPlanetByName(const char* planet_name) const {
    // Returns NULL if planet_name not found
    for(int i=0; i < planet_count; i++) {
        if (strcmp(planet_array[i].name, planet_name) == 0) {
            return &planet_array[i];
        }
    }
    return NULL;
}

const PlanetNature* Planets::GetParentNature() const {
    return &parent;
}

int Planets::LoadEphemerides(const DataNode* data) {
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
        timemath::Time epoch = timemath::TimeSub(GlobalGetNow(), timemath::Time(ann / sqrt(parent.mu / (sma*sma*sma))));

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

Planet* GetPlanet(entity_id_t id) { return GlobalGetState()->planets.GetPlanet(id); }
int LoadEphemerides(const DataNode* data) { return GlobalGetState()->planets.LoadEphemerides(data); }