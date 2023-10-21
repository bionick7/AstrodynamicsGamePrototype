#include "planet.h"
#include "global_state.h"


void PlanetClicked(Planet* planet) {
    TransferPlanAddPlanet(&(GetGlobalState()->active_transfer_plan), planet);
}

void PlanetDraw(Planet* planet, const DrawCamera* cam) {
    //printf("%f : %f\n", planet->position.x, planet->position.y);
    Vector2 screen_pos = CameraTransformV(cam, planet->position);
    DrawCircleV(screen_pos, CameraTransformS(cam, planet->radius), WHITE);
    DrawLineV(CameraTransformV(cam, (Vector2){0}), screen_pos, WHITE);

    SampleOrbit(&planet->orbit, &(planet->orbit_draw_buffer)[0], ORBIT_BUFFER_SIZE);
    CameraTransformBuffer(cam, &(planet->orbit_draw_buffer)[0], ORBIT_BUFFER_SIZE);
    DrawLineStrip(&(planet->orbit_draw_buffer)[0], ORBIT_BUFFER_SIZE, WHITE);
}

void PlanetDrawUI(Planet* planet, const DrawCamera* cam) {
    Vector2 screen_pos = CameraTransformV(cam, planet->position);
    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    int text_h = 16;
    int text_w = MeasureText(planet->name, text_h);
    DrawText(planet->name, screen_x - text_w / 2, screen_y - text_h - 5, text_h, WHITE);

    float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), screen_pos);
    if (mouse_dist_sqr < 20*20) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 10, RED);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlanetClicked(planet);
        }
    }
}