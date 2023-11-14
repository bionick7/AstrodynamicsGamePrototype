#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"

Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;
    orbit = OrbitFromElements(1, 0, 0, 1, 0, false);
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = 1e5;
        resource_delta[i] = 0;
        resource_capacity[i] = 1e7;
    }
    resource_delta[RESOURCE_FOOD] = -20;
}

void Planet::Load(const DataNode *data, double parent_mu) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    mu = data->GetF("mass", mu, true) * G;
    radius = data->GetF("radius", radius, true);

    if (data->Has("resources")) {
        int resource_count = data->GetArrayChildLen("resources", true);
        if (resource_count > RESOURCE_MAX) {
            resource_count = RESOURCE_MAX;
        }
        for (int resource_index=0; resource_index < resource_count; resource_index++) {
            DataNode* resource = data->GetArrayChild("resources", resource_index, true);
            resource_stock[resource_index] = resource->GetF("stock") * 1000;
            resource_delta[resource_index] = resource->GetF("delta") * 1000;
        }
    }

    if (data->Has("modules")) {
        module_count = data->GetArrayLen("modules", true);
        if (module_count > MAX_PLANET_MODULES) {
            module_count = MAX_PLANET_MODULES;
        }
        
        for (int i = 0; i < module_count; i++) {
            const char* module_id = data->GetArray("modules", i);
            modules[i] = GetModuleIndexById(module_id);
        }
    }

    double sma = data->GetF("SMA", orbit.sma, true);
    double epoch = orbit.epoch;
    if (data->Has("Ann")) {
        double ann = data->GetF("Ann", 0) * DEG2RAD;
        epoch = GlobalGetNow() - ann / sqrt(parent_mu / (sma*sma*sma));
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

    for (int i = 0; i < module_count; i++) {
        // TODO Update resource delta and stats
        if (modules[i] == MODULE_INDEX_INVALID) continue;
        const Module* module_ = GetModuleByIndex(modules[i]);
        if (module_ == NULL) continue;
        module_->Effect(resource_delta, stats);
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
    time_type now = GlobalGetNow();
    time_type prev = GlobalGetPreviousFrameTime();
    RecalcStats();
    position = OrbitGetPosition(&orbit, now);
    double delta_T = (now - prev) / 86400;
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

void Planet::DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer) {
    if (upper_quadrant) {
        UIContextCreate(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    } else {
        UIContextCreate(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    }
    UIContextCurrent().Enclose(2, 2, BG_COLOR, MAIN_UI_COLOR);
    UIContextWrite(name);
    UIContextWrite("================");
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        strcpy(buffer, resources_names[i]);
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", resources_names[i], qtt, cap, delta);
        sprintf(buffer, "%-10s %3.1fT (%+3d T/d)", resources_names[i], resource_stock[i] / 1e3, (int)(resource_delta[i]/1e3));
        if (GlobalGetState()->active_transfer_plan.IsActive()) {
            if (UIContextDirectButton(transfer.resource_id == i ? "X" : " ", 2) & BUTTON_STATE_FLAG_JUST_PRESSED) {
                GlobalGetState()->active_transfer_plan.SetResourceType(i);
            }
        }
        UIContextWrite(buffer, false);
        if (transfer.resource_id == i) {
            sprintf(buffer, "   %+3.1fK", transfer.quantity / 1000);
            UIContextWrite(buffer, false);
        }
        UIContextWrite("");
        //TextBoxLineBreak(&tb);
    }
}