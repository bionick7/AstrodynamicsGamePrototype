#include "global_state.hpp"
#include "ephemerides.hpp"
#include "debug_drawing.hpp"
#include <random>

GlobalState global_state;

bool IsIdValid(entity_id_t id) {
    if (id == entt::null) return false;
    if (!global_state.registry.valid(id)) return false;
    return true;
}

GlobalState* GlobalGetState() {
    return &global_state;
}

time_type GlobalGetPreviousFrameTime() {
    return global_state.prev_time;
}

time_type GlobalGetNow() {
    return global_state.time;
}

Ship& GetShip(entity_id_t id) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id")
    }
    return global_state.registry.get<Ship>(id);
}

Planet& GetPlanet(entity_id_t id) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id")
    }
    return global_state.registry.get<Planet>(id);
}

void GlobalState::_InspectState() {
    /*printf("%d planets, %d ships\n", planet_count, ship_count);
    printf("Orbits:\n");
    for (int i=0; i < planet_count; i++) {
        OrbitPrint(&planets[i].orbit);
        printf("\n");
    }
    for (int i=0; i < ship_count; i++) {
        ShipInspect(&ships[i]);
    }*/
}

entity_id_t GlobalState::_AddPlanet(
        const char* name, double mu_parent, 
        double sma, double ecc, double lop, double ann, bool is_prograde, 
        double radius, double mu
    ) {
    auto planet_entity = registry.create();
    //entity_map.insert({uuid, planet_entity});
    Planet &planet = registry.emplace<Planet>(planet_entity);

    planet.Make(name, mu, radius);
    planet.orbit = OrbitFromElements(sma, ecc, lop, mu_parent, time -ann / sqrt(mu_parent / (sma*sma*sma)), is_prograde);
    planet.id = planet_entity;
    planet.Update();
}

entity_id_t GlobalState::_AddShip(const char* name, entity_id_t origin_planet) {
    auto ship_entity = registry.create();
    //entity_map.insert({uuid, ship_entity});
    Ship &ship = registry.emplace<Ship>(ship_entity);

    ship.Make(name);
    ship.parent_planet = origin_planet;
    ship.current_state = SHIP_STATE_REST;
    ship.id = ship_entity;
    ship.Update();
}

void GlobalState::Make(time_type time) {
    time = time;
    TransferPlanUIMake(&active_transfer_plan);
    CameraMake(&camera);
}

void GlobalState::Load(const char * file_path) {
    entity_id_t planets[100];
    int num_planets = sizeof(planet_params) / (sizeof(planet_params[0]) * 6);
    if (num_planets > 100) num_planets = 100;
    for(int i=0; i < num_planets; i++) {
        planets[i] = _AddPlanet(
            planet_names[i], mu_parent,
            planet_params[i*6], planet_params[i*6+1], planet_params[i*6+2], planet_params[i*6+3], (planet_orbit_is_prograde >> i) % 2,
            planet_params[i*6+4], planet_params[i*6+5]
        );
    }
    char name[] = "Ship 0";
    for(int i=0; i < 10; i++) {
        _AddShip(name, planets[2]);
        name[5]++;
    }
    _InspectState();
}

// Update
void GlobalState::UpdateState(double delta_t) {
    DebugClearText();

    CameraHandleInput(&camera, delta_t);
    TransferPlanUIUpdate(&active_transfer_plan);
    prev_time = time;
    time = CameraAdvanceTime(&camera, time, delta_t);

    Clickable hover = {TYPE_NONE, GetInvalidId()};
    double min_distance = INFINITY;

    //int ships_per_planet[MAX_PLANETS] = {0};

    auto ship_view = registry.view<Ship>();
    auto planet_view = registry.view<Planet>();

    for (auto [_, planet] : planet_view.each()) {
        planet.Update();
        planet.mouse_hover = false;
        if (planet.HasMouseHover(&min_distance)) {
            hover = (Clickable) {TYPE_PLANET, planet.id};
        }
    }
    int total_ship_count = 0;
    for (auto [_, ship] : ship_view.each()) {
        ship.Update();
        if (IsIdValid(ship.parent_planet)) {
            ship.index_on_planet = total_ship_count++;
        }
        ship.mouse_hover = false;
        if (ship.HasMouseHover(&min_distance)) {
            hover = (Clickable) {TYPE_SHIP, ship.id};
        }
    }
    switch (hover.type) {
    case TYPE_PLANET: GetPlanet(hover.id).mouse_hover = true; break;
    case TYPE_SHIP: GetShip(hover.id).mouse_hover = true; break;
    case TYPE_NONE:
    default: break;
    }
}

// Draw
void GlobalState::DrawState() {
    DrawCamera* cam = &camera;

    auto planet_view = registry.view<Planet>();
    auto ship_view = registry.view<Ship>();

    DrawCircleV(CameraTransformV(cam, (Vector2){0}), CameraTransformS(cam, radius_parent), MAIN_UI_COLOR);
    for (auto [_, planet] : planet_view.each()) {
        planet.Draw(cam);
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.Draw(cam);
    }

    TransferPlanUIDraw(&active_transfer_plan, cam);

    // UI
    CameraDrawUI(cam);
    for (auto [_, planet] : planet_view.each()) {
        if (active_transfer_plan.plan.departure_planet == planet.id) {
            planet.DrawUI(cam, true, ResourceTransferInvert(active_transfer_plan.plan.resource_transfer));
        }
        if (active_transfer_plan.plan.arrival_planet == planet.id) {
            planet.DrawUI(cam, false, active_transfer_plan.plan.resource_transfer);
        }
        if (!IsIdValid(active_transfer_plan.plan.departure_planet) < 0 && planet.mouse_hover) {
            planet.DrawUI(cam, true, EMPTY_TRANSFER);
        }
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.DrawUI(cam);
    }
    //DrawFPS(0, 0);
}
