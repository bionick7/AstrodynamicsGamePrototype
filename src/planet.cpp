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
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = 1e5;
        resource_delta[i] = 0;
        resource_capacity[i] = 1e7;
    }
}

void Planet::Load(const DataNode *data, double parent_mu) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    mu = data->GetF("mass", mu, true) * G;
    radius = data->GetF("radius", radius, true);

    const DataNode* resource_node = data->GetChild("resource_stock", true);
    if (resource_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            resource_stock[resource_index] = resource_node->GetF(resources_names[resource_index], 0, true) * 1000;
        }
    }

    if (data->Has("modules")) {
        int initial_module_count = data->GetArrayLen("modules", true);
        if (initial_module_count > MAX_PLANET_MODULES) {
            initial_module_count = MAX_PLANET_MODULES;
        }
        
        for (int i = 0; i < initial_module_count; i++) {
            const char* module_id = data->GetArray("modules", i);
            modules[i] = ModuleInstance(GetModuleIndexById(module_id));
        }
    }

    double sma = data->GetF("SMA", orbit.sma, true);
    Time epoch = orbit.epoch;
    if (data->Has("Ann")) {
        double ann = data->GetF("Ann", 0) * DEG2RAD;
        epoch = TimeSub(GlobalGetNow(), Time(ann / sqrt(parent_mu / (sma*sma*sma))));
    }
    orbit = OrbitFromElements(
        sma,
        data->GetF("Ecc", orbit.ecc, true),
        (data->GetF("LoA", orbit.lop * RAD2DEG, true) + data->GetF("AoP", 0, true)) * DEG2RAD,
        parent_mu,
        epoch, 
        strcmp(data->Get("retrograde", orbit.prograde ? "n" : "y", true), "y") != 0
    );
}


void Planet::Save(DataNode* data) const {

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

    for (int i = 0; i < MAX_PLANET_MODULES; i++) {
        if (modules[i].IsValid()) {
            modules[i].Effect(&resource_delta[0], &stats[0]);
        }
    }
}

void Planet::RequestBuild(int slot, module_index_t module_class) {
    const ModuleClass* mc = GetModuleByIndex(module_class);
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
    ModuleInstance instance = ModuleInstance(module_class);
    modules[slot] = instance;

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

    // Draw modules
    int current_width = UIContextCurrent().width;
    UIContextPushInset(0, UIContextCurrent().height - UIContextCurrent().y_cursor);
    UIContextPushHSplit(0, current_width/2);
    for (int i = 0; i < MAX_PLANET_MODULES; i += 2) {
        if (modules[i].UIDraw()) {
            ModuleConstructionOpen(id, i);
        }
    }
    UIContextPop();  // HSplit
    UIContextPushHSplit(current_width/2, current_width);
    for (int i = 1; i < MAX_PLANET_MODULES; i += 2) {
        if (modules[i].UIDraw()) {
            ModuleConstructionOpen(id, i);
        }
    }
    UIContextPop();  // HSplit
    UIContextPop();  // Inset
}