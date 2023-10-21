#include "global_state.h"
#include "ephemerides.h"
#include "debug_drawing.h"

GlobalState global_state = {0};

GlobalState* GetGlobalState() {
    return &global_state;
}

void _InspectState(GlobalState* gs) {
    printf("%d planets, %d ships\n", gs->planet_count, gs->ship_count);
    printf("Orbits:\n");
    for (int i=0; i < gs->planet_count; i++) {
        Planet* p = &gs->planets[i];
        OrbitPrint(&gs->planets[i].orbit);
        printf("\n");
    }
    for (int i=0; i < gs->ship_count; i++) {
        Ship* s = &gs->ships[i];
        OrbitPrint(&gs->ships[i].orbit);
        printf("\n");
    }
}

void _AddPlanet(
        GlobalState* gs, const char* name, double mu_parent, 
        double sma, double ecc, double lop, double ann, bool is_prograde, 
        double radius, double mu
    ) {
    gs->planets[gs->planet_count] = (Planet) {0};
    Planet* planet = &gs->planets[gs->planet_count];
    planet->name = name;
    planet->mu = mu;
    planet->orbit = OrbitFromElements(sma, ecc, lop, mu_parent, gs->time -ann / sqrt(mu_parent / (sma*sma*sma)), is_prograde);
    planet->radius = radius;
    UpdateOrbit(&planet->orbit, gs->time, &planet->position, &planet->velocity);
    gs->planet_count++;
}

// Lifecycle
void MakeGlobalState(GlobalState* gs, time_type time) {
    gs->planet_count = 0;
    gs->ship_count = 0;
    gs->camera = (DrawCamera){0};
    MakeCamera(&gs->camera);
}

void LoadGlobalState(GlobalState* gs, const char* file_path) {
    for(int i=0; i < sizeof(planet_params) / (sizeof(planet_params[0]) * 6); i++) {
        _AddPlanet(gs,
            planet_names[i], mu_parent,
            planet_params[i*6], planet_params[i*6+1], planet_params[i*6+2], planet_params[i*6+3], (planet_orbit_is_prograde >> i) % 2,
            planet_params[i*6+4], planet_params[i*6+5]
        );
    }
    _InspectState(gs);
}

void DestroyGlobalState(GlobalState* gs) {

}

// Update
void UpdateState(GlobalState* gs, double delta_t) {
    DebugClearText();

    CameraHandleInput(&gs->camera, delta_t);
    TransferPlanUpdate(&gs->active_transfer_plan);
    gs->time = CameraAdvanceTime(&gs->camera, gs->time, delta_t);
    for (int i=0; i < gs->planet_count; i++) {
        Planet* p = &gs->planets[i];
        UpdateOrbit(&p->orbit, gs->time, &p->position, &p->velocity);
    }
    for (int i=0; i < gs->ship_count; i++) {
        Ship* s = &gs->ships[i];
        UpdateOrbit(&s->orbit, gs->time, &s->position, &s->velocity);
    }
}

// Draw
void DrawState(GlobalState* gs) {
    DrawCamera* cam = &gs->camera;

    //DrawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, CameraTransformS(cam, radius_parent), WHITE);
    for (int i=0; i < gs->planet_count; i++) {
        PlanetDraw(&gs->planets[i], cam);
    }
    for (int i=0; i < gs->ship_count; i++) {
        Ship* s = &gs->ships[i];
        DrawCircleV(CameraTransformV(cam, s->position), 10, RED);
    }
    TransferPlanDraw(&gs->active_transfer_plan, cam);

    // UI
    for (int i=0; i < gs->planet_count; i++) {
        PlanetDrawUI(&gs->planets[i], cam);
    }
    DrawFPS(0, 0);
    CameraDrawUI(cam);
}
