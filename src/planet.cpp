#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt) {
    rt.quantity = -rt.quantity;
    return rt;
}

void PlanetMake(Planet* planet, const char* name, double mu, double radius) {
    strcpy(planet->name, name);
    planet->mu = mu;
    planet->radius = radius;
    for (int i=0; i < RESOURCE_MAX; i++) {
        planet->resource_stock[i] = 1e5;
        planet->resource_delta[i] = 0;
        planet->resource_capacity[i] = 1e6;
    }
}

void _PlanetClicked(Planet* planet) {
    if (GlobalGetState()->active_transfer_plan.plan.arrival_planet == -1) {
        TransferPlanUISetDestination(&(GlobalGetState()->active_transfer_plan), planet->id);
    } else {
        GetMainCamera()->focus = planet->position.cartesian;
    }
}

double PlanetScreenRadius(const Planet* planet) {
    return fmax(CameraTransformS(GetMainCamera(), planet->radius), 4);
}

double PlanetGetDVFromExcessVelocity(const Planet* planet, Vector2 vel) {
    return sqrt(planet->mu / (2*planet->radius) + Vector2LengthSqr(vel));
}

resource_count_t PlanetDrawResource(Planet* planet, int resource_id, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to give
    if (resource_id < 0 || resource_id >= RESOURCE_MAX) {
        return 0;
    }
    
    resource_count_t transferred_resources = ClampInt(quantity, 0, planet->resource_stock[resource_id]);
    planet->resource_stock[resource_id] -= transferred_resources;

    return transferred_resources;
}

resource_count_t PlanetGiveResource(Planet* planet, int resource, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to take
    if (resource < 0 || resource >= RESOURCE_MAX) {
        return 0;
    }

    resource_count_t transferred_resources = Clamp(quantity, 0, planet->resource_capacity[resource]);
    planet->resource_stock[resource] += transferred_resources;

    return transferred_resources;
}

bool PlanetHasMouseHover(const Planet* planet, double* min_distance) {
    Vector2 screen_pos = CameraTransformV(GetMainCamera(), planet->position.cartesian);
    double dist = Vector2Distance(GetMousePosition(), screen_pos);
    if (dist <= PlanetScreenRadius(planet) * 1.2 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}

void PlanetUpdate(Planet *planet) {
    time_type now = GlobalGetNow();
    time_type prev = GlobalGetPreviousFrameTime();
    planet->position = OrbitGetPosition(&planet->orbit, now);
    double delta_T = (now - prev) / 86400;
    for (int i=0; i < RESOURCE_MAX; i++) {
        planet->resource_stock[i] = Clamp(planet->resource_stock[i] + planet->resource_delta[i] * delta_T, 0, planet->resource_capacity[i]);
    }
}

void PlanetDraw(Planet* planet, const DrawCamera* cam) {
    //printf("%f : %f\n", planet->position.x, planet->position.y);
    Vector2 screen_pos = CameraTransformV(cam, planet->position.cartesian);
    DrawCircleV(screen_pos, PlanetScreenRadius(planet), MAIN_UI_COLOR);
    //DrawLineV(CameraTransformV(cam, (Vector2){0}), screen_pos, MAIN_UI_COLOR);
    
    DrawOrbit(&planet->orbit, MAIN_UI_COLOR);

    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    int text_h = 16;
    Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), planet->name, text_h, 1);
    DrawTextEx(GetCustomDefaultFont(), planet->name, (Vector2) {screen_x - text_size.x / 2,  screen_y - text_size.y - 5}, text_h, 1, MAIN_UI_COLOR);

    if (planet->mouse_hover) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 10, TRANSFER_UI_COLOR);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _PlanetClicked(planet);
        }
    }
}

void PlanetDrawUI(Planet* planet, const DrawCamera* cam, bool upper_quadrant, ResourceTransfer transfer) {
    TextBox tb;
    if (upper_quadrant) {
        tb = TextBoxMake(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);
    } else {
        tb = TextBoxMake(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, MAIN_UI_COLOR);  // TODO
    }
    TextBoxEnclose(&tb, 2, 2, BG_COLOR, MAIN_UI_COLOR);
    TextBoxWriteLine(&tb, planet->name);
    TextBoxWriteLine(&tb, "================");
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        strcpy(buffer, resources_names[i]);
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", resources_names[i], qtt, cap, delta);
        sprintf(buffer, "%-10s %3.1fK (%+3d)", resources_names[i], planet->resource_stock[i] / 1000, (int)planet->resource_delta[i]);
        if (TransferPlanUIIsActive(&GlobalGetState()->active_transfer_plan)) {
            if (TextBoxWriteButton(&tb, transfer.resource_id == i ? "X" : " ", 2) & BUTTON_JUST_PRESSED) {
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