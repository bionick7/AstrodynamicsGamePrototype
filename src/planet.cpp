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
    if (GlobalGetState()->active_transfer_plan.plan.arrival_planet == entt::null) {
        TransferPlanUISetDestination(&(GlobalGetState()->active_transfer_plan), id);
    } else {
        GetMainCamera()->focus = position.cartesian;
    }
}

double Planet::ScreenRadius() const {
    return fmax(CameraTransformS(GetMainCamera(), radius), 4);
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
    Vector2 screen_pos = CameraTransformV(GetMainCamera(), position.cartesian);
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

void Planet::Draw(const DrawCamera* cam) {
    //printf("%f : %f\n", position.x, position.y);
    Vector2 screen_pos = CameraTransformV(cam, position.cartesian);
    DrawCircleV(screen_pos, ScreenRadius(), MAIN_UI_COLOR);
    //DrawLineV(CameraTransformV(cam, (Vector2){0}), screen_pos, MAIN_UI_COLOR);
    
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

void Planet::DrawUI(const DrawCamera* cam, bool upper_quadrant, ResourceTransfer transfer) {
    TextBox tb;
    if (upper_quadrant) {
        tb = TextBoxMake(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    } else {
        tb = TextBoxMake(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);  // TODO
    }
    TextBoxEnclose(&tb, 2, 2, BG_COLOR, MAIN_UI_COLOR);
    TextBoxWriteLine(&tb, name);
    TextBoxWriteLine(&tb, "================");
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        strcpy(buffer, resources_names[i]);
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", resources_names[i], qtt, cap, delta);
        sprintf(buffer, "%-10s %3.1fT (%+3d T/d)", resources_names[i], resource_stock[i] / 1e3, (int)(resource_delta[i]/1e3));
        if (TransferPlanUIIsActive(&GlobalGetState()->active_transfer_plan)) {
            if (TextBoxWriteButton(&tb, transfer.resource_id == i ? "X" : " ", 2) & BUTTON_STATE_FLAG_JUST_PRESSED) {
                TransferPlanUISetResourceType(&GlobalGetState()->active_transfer_plan, i);
            }
        }
        TextBoxWrite(&tb, buffer);
        if (transfer.resource_id == i) {
            sprintf(buffer, "   %+3.1fK", transfer.quantity / 1000);
            TextBoxWrite(&tb, buffer);
        }
        TextBoxWriteLine(&tb, "");
        //TextBoxLineBreak(&tb);
    }
}