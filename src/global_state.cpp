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

entity_id_t GlobalState::_AddPlanet(int index) {
    printf("Planet N°%d\n", index);
    const char* name = PLANET_NAMES[index];
    double sma = PLANET_TABLE[index*6];
    double ecc = PLANET_TABLE[index*6+1];
    double lop = PLANET_TABLE[index*6+2];
    double ann = PLANET_TABLE[index*6+3];
    bool is_prograde = (PLANET_PROGRADE_FLAGS >> index) % 2;
    double radius = PLANET_TABLE[index*6+4];
    double mu = PLANET_TABLE[index*6+5];

    auto planet_entity = registry.create();
    //entity_map.insert({uuid, planet_entity});
    Planet &planet = registry.emplace<Planet>(planet_entity);

    planet.Make(name, mu, radius);
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        planet.resource_stock[resource_index] = PLANET_RESOURCE_STOCK_TABLE[index * RESOURCE_MAX + resource_index] * 1000;
        planet.resource_delta[resource_index] = PLANET_RESOURCE_DELTA_TABLE[index * RESOURCE_MAX + resource_index] * 1000;
    }
    planet.orbit = OrbitFromElements(sma, ecc, lop, PARENT_MU, time -ann / sqrt(PARENT_MU / (sma*sma*sma)), is_prograde);
    planet.id = planet_entity;
    planet.Update();
    return planet_entity;
}

entity_id_t GlobalState::_AddShip(int index, entity_id_t origin_planet) {
    printf("Ship N°%d\n", index);
    const char* name = SHIP_NAMES[index];

    auto ship_entity = registry.create();
    //entity_map.insert({uuid, ship_entity});
    Ship &ship = registry.emplace<Ship>(ship_entity);

    ship.Make(name);
    ship.max_capacity = SHIP_STAT_TABLE[index*3] * 1000;
    ship.max_dv = SHIP_STAT_TABLE[index*3 + 1] * 1000;
    ship.v_e = SHIP_STAT_TABLE[index*3 + 2] * 1000;
    ship.parent_planet = origin_planet;
    ship.id = ship_entity;
    ship.Update();
    return ship_entity;
}

void GlobalState::Make(time_type time) {
    time = time;
    TransferPlanUIMake(&active_transfer_plan);
    c_transf.Make();
}

void GlobalState::Load(const char * file_path) {
    entity_id_t planets[100];
    int num_planets = sizeof(PLANET_NAMES) / (sizeof(PLANET_NAMES[0]));
    int num_ships = sizeof(SHIP_NAMES) / (sizeof(SHIP_NAMES[0]));
    if (num_planets > 100) num_planets = 100;
    for(int i=0; i < num_planets; i++) {
        planets[i] = _AddPlanet(i);
    }
    for(int i=0; i < num_ships; i++) {
        _AddShip(i, planets[2]);
    }
    _InspectState();
}

// Update
void GlobalState::UpdateState(double delta_t) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    c_transf.HandleInput(delta_t);
    TransferPlanUIUpdate(&active_transfer_plan);
    prev_time = time;
    time = c_transf.AdvanceTime(time, delta_t);

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

    auto planet_view = registry.view<Planet>();
    auto ship_view = registry.view<Ship>();

    DrawCircleV(c_transf.TransformV((Vector2){0}), c_transf.TransformS(PARENT_RADIUS), MAIN_UI_COLOR);
    for (auto [_, planet] : planet_view.each()) {
        planet.Draw(&c_transf);
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.Draw(&c_transf);
    }

    TransferPlanUIDraw(&active_transfer_plan, &c_transf);

    // UI
    c_transf.DrawUI();
    for (auto [_, planet] : planet_view.each()) {
        if (active_transfer_plan.plan.departure_planet == planet.id) {
            planet.DrawUI(&c_transf, true, ResourceTransferInvert(active_transfer_plan.plan.resource_transfer));
        }
        if (active_transfer_plan.plan.arrival_planet == planet.id) {
            planet.DrawUI(&c_transf, false, active_transfer_plan.plan.resource_transfer);
        }
        if (!TransferPlanUIIsActive(&active_transfer_plan) && planet.mouse_hover) {
            planet.DrawUI(&c_transf, true, EMPTY_TRANSFER);
        }
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.DrawUI(&c_transf);
    }

    DebugFlushText();
    //DrawFPS(0, 0);
}
