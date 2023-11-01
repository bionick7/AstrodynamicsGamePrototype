#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt) {
    rt.quantity = -rt.quantity;
    return rt;
}

void Planet::Make(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = 1e5;
        resource_delta[i] = 0;
        resource_capacity[i] = 1e7;
    }
    resource_delta[RESOURCE_FOOD] = -20;
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
    DrawTextEx(GetCustomDefaultFont(), name, (Vector2) {screen_x - text_size.x / 2,  screen_y - text_size.y - 5}, text_h, 1, MAIN_UI_COLOR);

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