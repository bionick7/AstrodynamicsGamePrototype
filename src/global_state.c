#include "global_state.h"
#include "ephemerides.h"
#include "debug_drawing.h"

GlobalState global_state = {0};

GlobalState* GlobalGetState() {
    return &global_state;
}

time_type GlobalGetNow() {
    return global_state.time;
}


Ship* GetShip(int id) {
    if (id < 0 || id >= global_state.ship_count) {
        FAIL("Invalid ship id")
    }
    return &global_state.ships[id];
}

Planet* _GetPlanet(int id, const char* file, int line) {
    if (id < 0 || id >= global_state.planet_count) {
        //FAIL_FORMAT("Invalid planet id (%d)", id)
        FAIL_FORMAT("Invalid planet id (%d) : %s:%d", id, file, line)
    }
    return &global_state.planets[id];
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
        ShipInspect(&gs->ships[i]);
    }
}

void _AddPlanet(
        GlobalState* gs, const char* name, double mu_parent, 
        double sma, double ecc, double lop, double ann, bool is_prograde, 
        double radius, double mu
    ) {
    gs->planets[gs->planet_count] = (Planet) {0};
    Planet* planet = &gs->planets[gs->planet_count];
    strcpy(planet->name, name);
    planet->mu = mu;
    planet->orbit = OrbitFromElements(sma, ecc, lop, mu_parent, gs->time -ann / sqrt(mu_parent / (sma*sma*sma)), is_prograde);
    planet->radius = radius;
    planet->id = gs->planet_count;
    PlanetUpdate(planet);
    gs->planet_count++;
}

void _AddShip(GlobalState* gs, const char* name, double dv, int origin_planet) {
    gs->ships[gs->ship_count] = (Ship) {0};
    Ship* ship = &gs->ships[gs->ship_count];
    strcpy(ship->name, name);
    ship->dv = dv;
    ship->max_dv = dv;
    ship->parent_planet = origin_planet;
    ship->current_state = SHIP_STATE_REST;
    ship->id = gs->ship_count;
    ShipUpdate(ship);
    gs->ship_count++;
}

// Lifecycle
void GlobalStateMake(GlobalState* gs, time_type time) {
    gs->planet_count = 0;
    gs->ship_count = 0;
    gs->time = time;
    TransferPlanUIMake(&gs->active_transfer_plan);
    CameraMake(&gs->camera);
}

void LoadGlobalState(GlobalState* gs, const char* file_path) {
    for(int i=0; i < sizeof(planet_params) / (sizeof(planet_params[0]) * 6); i++) {
        _AddPlanet(gs,
            planet_names[i], mu_parent,
            planet_params[i*6], planet_params[i*6+1], planet_params[i*6+2], planet_params[i*6+3], (planet_orbit_is_prograde >> i) % 2,
            planet_params[i*6+4], planet_params[i*6+5]
        );
    }
    char name[] = "Ship 0";
    for(int i=0; i < 10; i++) {
        _AddShip(gs, name, 10000, 2);
        name[5]++;
    }
    _InspectState(gs);
}

void DestroyGlobalState(GlobalState* gs) {

}

// Update
void UpdateState(GlobalState* gs, double delta_t) {
    DebugClearText();

    CameraHandleInput(&gs->camera, delta_t);
    TransferPlanUIUpdate(&gs->active_transfer_plan);
    gs->time = CameraAdvanceTime(&gs->camera, gs->time, delta_t);

    Clickable hover = {TYPE_NONE, -1};
    double min_distance = INFINITY;

    int ships_per_planet[MAX_PLANETS] = {0};

    for (int i=0; i < gs->planet_count; i++) {
        PlanetUpdate(&gs->planets[i]);
        gs->planets[i].mouse_hover = false;
        if (PlanetHasMouseHover(&gs->planets[i], &min_distance)) {
            hover = (Clickable) {TYPE_PLANET, i};
        }
    }
    for (int i=0; i < gs->ship_count; i++) {
        ShipUpdate(&gs->ships[i]);
        if (gs->ships[i].parent_planet >= 0) {
            gs->ships[i].index_on_planet = ships_per_planet[gs->ships[i].parent_planet]++;
        }
        gs->ships[i].mouse_hover = false;
        if (ShipHasMouseHover(&gs->ships[i], &min_distance)) {
            hover = (Clickable) {TYPE_SHIP, i};
        }
    }
    switch (hover.type) {
    case TYPE_PLANET: GetPlanet(hover.id)->mouse_hover = true; break;
    case TYPE_SHIP: GetShip(hover.id)->mouse_hover = true; break;
    }
}

// Draw
void DrawState(GlobalState* gs) {
    DrawCamera* cam = &gs->camera;

    DrawCircleV(CameraTransformV(cam, (Vector2){0}), CameraTransformS(cam, radius_parent), WHITE);
    for (int i=0; i < gs->planet_count; i++) {
        PlanetDraw(&gs->planets[i], cam);
    }
    for (int i=0; i < gs->ship_count; i++) {
        ShipDraw(&gs->ships[i], cam);
    }

    // UI
    for (int i=0; i < gs->planet_count; i++) {
        PlanetDrawUI(&gs->planets[i], cam);
    }
    TransferPlanUIDraw(&gs->active_transfer_plan, cam);
    DrawFPS(0, 0);
    CameraDrawUI(cam);
}
