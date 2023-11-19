#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "constants.hpp"

GlobalState global_state;

bool IsIdValid(entity_id_t id) {
    if (id == entt::null) return false;
    if (!global_state.registry.valid(id)) return false;
    return true;
}

GlobalState* GlobalGetState() {
    return &global_state;
}

Time GlobalGetPreviousFrameTime() {
    return global_state.prev_time;
}

Time GlobalGetNow() {
    return global_state.time;
}

Ship& GetShip(entity_id_t id) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id")
    }
    return global_state.registry.get<Ship>(id);
}

Planet& _GetPlanet(entity_id_t id, const char* file_name, int line) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id (called from %s:%d)", file_name, line)
    }
    return global_state.registry.get<Planet>(id);
}

Planet* GetPlanetByName(const char* planet_name) {
    // Returns NULL if planet_name not found
    auto planet_view = global_state.registry.view<Planet>();
    for (auto [_, planet] : planet_view.each()) {
        if (strcmp(planet.name, planet_name) == 0) {
            return &planet;
        }
    }
    return NULL;
}

void GlobalState::_InspectState() {

}

entity_id_t _AddPlanet(GlobalState* gs, const DataNode* data, double parent_mu) {
    entity_id_t planet_entity = gs->registry.create();
    //entity_map.insert({uuid, planet_entity});
    Planet &planet = gs->registry.emplace<Planet>(planet_entity);
    planet.Load(data, parent_mu);
    planet.id = planet_entity;
    planet.Update();

    return planet_entity;
}

entity_id_t _AddShip(GlobalState* gs, const DataNode* data) {
    //printf("Adding Ship N°%d\n", index)

    auto ship_entity = gs->registry.create();
    //entity_map.insert({uuid, ship_entity});
    Ship &ship = gs->registry.emplace<Ship>(ship_entity);
    
    ship.Load(data);
    const char* planet_name = data->Get("planet", "NO NAME SPECIFIED");
    const Planet* planet = GetPlanetByName(planet_name);
    if (planet == NULL) {
        FAIL("Error while initializing ship '%s': no such planet '%s'", ship.name, planet_name)
    }
    ship.parent_planet = planet->id;
    ship.id = ship_entity;
    ship.Update();
    return ship_entity;
}

void GlobalState::Make(Time p_time) {
    time = p_time;
    active_transfer_plan.Make();
    c_transf.Make();
    focused_planet = GetInvalidId();
    focused_ship = GetInvalidId();
}

void GlobalState::LoadConfigs(const char* ephemerides_path, const char* module_data_path) {
    DataNode ephemerides;
    DataNode module_data;
    INFO("Loading Config")
    if (DataNode::FromFile(&ephemerides, ephemerides_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", ephemerides_path);
    }
    if (DataNode::FromFile(&module_data, module_data_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", module_data_path);
    }

    // Init planets
    parent_radius = ephemerides.GetF("radius");
    parent_mu = ephemerides.GetF("mass") * G;
    int num_planets = ephemerides.GetArrayChildLen("satellites");
    if (num_planets > 100) num_planets = 100;
    //entity_id_t planets[100];
    for(int i=0; i < num_planets; i++) {
        const DataNode* data = ephemerides.GetArrayChild("satellites", i);
        _AddPlanet(this, data, parent_mu);
        //printf("%d\n", planets[i]);
    }
    
    int num_modules = LoadModules(&module_data);

    // TODO: load modules into more comprehensive memory

    INFO("%d planets, %d modules", num_planets, num_modules)
}

void GlobalState::LoadGame(const char* file_path) {
    DataNode game_data = DataNode();
    if (DataNode::FromFile(&game_data, file_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", file_path);
    }

    //planet_data.Inspect();

    int num_ships = game_data.GetArrayChildLen("ships");
    for(int i=0; i < num_ships; i++) {
        _AddShip(this, game_data.GetArrayChild("ships", i));
    }
    const DataNode* planets = game_data.GetChild("planets", true);
    if (planets != NULL){
        for(int i=0; i < planets->GetChildCount(); i++) {
            const char* planet_name = planets->GetChildKey(i);
            const DataNode* planet_data = planets->GetChild(planet_name);
            Planet* planet = GetPlanetByName(planet_name);
            if (planet == NULL) { 
                FAIL("Error while loading game at '%s': no such planet '%s'", file_path, planet_name)
            }
            planet->Load(planet_data, parent_mu);
        }
    }
    _InspectState();
}

// Update
void GlobalState::UpdateState(double delta_t) {
    c_transf.HandleInput(delta_t);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        // Cancel out of next layer

        // cancel out of focused planet and ship
        if (ModuleConstructionIsOpen()) {
            ModuleConstructionClose();
        } 

        // transfer plan UI
        else if (active_transfer_plan.IsActive()) {
            active_transfer_plan.Abort();
        } 
        // cancel out of focused planet and ship
        else if (IsIdValid(focused_planet) || IsIdValid(focused_ship)) {
            focused_planet = GetInvalidId();
            focused_ship = GetInvalidId();
        } else {
            // Enter Pause menu
        }
    }

    active_transfer_plan.Update();
    prev_time = time;
    time = c_transf.AdvanceTime(time, delta_t);

    Clickable hover = {TYPE_NONE, GetInvalidId()};
    double min_distance = INFINITY;

    auto ship_view = registry.view<Ship>();
    auto planet_view = registry.view<Planet>();

    for (auto [_, planet] : planet_view.each()) {
        planet.Update();
        planet.mouse_hover = false;
        if (planet.HasMouseHover(&min_distance)) {
            hover = {TYPE_PLANET, planet.id};
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
            hover = {TYPE_SHIP, ship.id};
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

    DrawCircleV(c_transf.TransformV({0}), c_transf.TransformS(parent_radius), MAIN_UI_COLOR);
    for (auto [_, planet] : planet_view.each()) {
        planet.Draw(&c_transf);
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.Draw(&c_transf);
    }

    active_transfer_plan.Draw(&c_transf);

    // UI
    UIStart();
    c_transf.DrawUI();



    // 
    for (auto [_, planet] : planet_view.each()) {
        if (active_transfer_plan.IsActive()){
            if (active_transfer_plan.plan->departure_planet == planet.id) {
                planet.DrawUI(&c_transf, true, ResourceTransferInvert(active_transfer_plan.plan->resource_transfer));
            }
            if (active_transfer_plan.plan->arrival_planet == planet.id) {
                planet.DrawUI(&c_transf, false, active_transfer_plan.plan->resource_transfer);
            }
        }
        else if (planet.mouse_hover || planet.id == focused_planet) {
            planet.DrawUI(&c_transf, true, EMPTY_TRANSFER);
        }
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.DrawUI(&c_transf);
    }
    ModuleConstructionUI();
    UIEnd();

    DebugFlushText();
    //DrawFPS(0, 0);
}
