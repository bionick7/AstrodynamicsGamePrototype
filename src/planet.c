#include "planet.h"
#include "global_state.h"


void _PlanetClicked(Planet* planet) {
    TransferPlanUISetDestination(&(GlobalGetState()->active_transfer_plan), planet->id);
}

double PlanetGetDVFromExcessVelocity(Planet* planet, Vector2 vel) {
    return sqrt(planet->mu / (2*planet->radius) + Vector2LengthSqr(vel));
}

void PlanetUpdate(Planet *planet) {
    planet->position = OrbitGetPosition(&planet->orbit, GlobalGetNow());
}

void PlanetDraw(Planet* planet, const DrawCamera* cam) {
    //printf("%f : %f\n", planet->position.x, planet->position.y);
    Vector2 screen_pos = CameraTransformV(cam, planet->position.cartesian);
    DrawCircleV(screen_pos, 
        fmax(CameraTransformS(cam, planet->radius), 4), 
    WHITE);
    //DrawLineV(CameraTransformV(cam, (Vector2){0}), screen_pos, WHITE);
    
    DrawOrbit(&planet->orbit, WHITE);
}

void PlanetDrawUI(Planet* planet, const DrawCamera* cam) {
    Vector2 screen_pos = CameraTransformV(cam, planet->position.cartesian);
    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    int text_h = 16;
    int text_w = MeasureText(planet->name, text_h);
    DrawText(planet->name, screen_x - text_w / 2, screen_y - text_h - 5, text_h, WHITE);

    float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), screen_pos);
    if (mouse_dist_sqr < 20*20) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 10, RED);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _PlanetClicked(planet);
        }
    }
}