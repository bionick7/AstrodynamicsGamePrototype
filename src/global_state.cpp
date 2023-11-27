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
    return global_state.calendar.prev_time;
}

Time GlobalGetNow() {
    return global_state.calendar.time;
}

Ship& GetShip(entity_id_t id) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id (%d)", id)
    }
    if (!global_state.registry.any_of<Ship>(id)) {
        FAIL("Id not ship (%d)", id)
    }
    return global_state.registry.get<Ship>(id);
}

Planet& _GetPlanet(entity_id_t id, const char* file_name, int line) {
    if ((!global_state.registry.valid(id))) {
        FAIL("Invalid id (%d) (called from %s:%d)", id, file_name, line)
    }
    if (!global_state.registry.any_of<Planet>(id)) {
        FAIL("Id (%d) not planet (called from %s:%d)", id, file_name, line)
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

bool _PauseMenuButton(const char* label) {
    UIContextPushInset(0, 20);
    UIContextEnclose(0, 0, BG_COLOR, MAIN_UI_COLOR);
    UIContextWrite(label);
    ButtonStateFlags buttonstate = UIContextAsButton();
    UIContextPop();
    return buttonstate & BUTTON_STATE_FLAG_JUST_PRESSED;
}

bool is_in_pause_menu;
void _PauseMenu() {
    const int menu_width = 200;
    const int button_height = 20;
    const int menu_height = button_height * 3;
    UIContextCreate(
        (GetScreenWidth() - menu_width)/2, 
        (GetScreenHeight() - menu_height)/2, 
        menu_width,
        menu_height,
        16,
        MAIN_UI_COLOR
    );
    UIContextEnclose(0, 0, BG_COLOR, MAIN_UI_COLOR);
    if (_PauseMenuButton("Save")) {
        INFO("Save")
        DataNode dn;
        GlobalGetState()->Serialize(&dn);
        dn.WriteToFile("save.yaml", FileFormat::YAML);
    }
    if (_PauseMenuButton("Load")) {
        INFO("Load")
        DataNode dn;
        DataNode::FromFile(&dn, "save.yaml", FileFormat::YAML, true, false);
        GlobalGetState()->Deserialize(&dn);
    }
    if (_PauseMenuButton("Exit")) {
        // ???
    }
}

void GlobalState::Make(Time p_time) {
    calendar.Make(p_time);
    active_transfer_plan.Make();
    c_transf.Make();
    focused_planet = GetInvalidId();
    focused_ship = GetInvalidId();
    capital = 0;
}

void GlobalState::LoadData() {
    const char* buildings_data_path = "resources/data/buildings.yaml";
    const char* ephemerides_path    = "resources/data/ephemerides.yaml";
    const char* ship_classes_path   = "resources/data/ship_classes.yaml";
    const char* resources_path      = "resources/data/resources.yaml";
    
    DataNode building_data;
    INFO("Loading Buildings")
    if (DataNode::FromFile(&building_data, buildings_data_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", buildings_data_path);
    }

    int num_buildings = LoadBuildings(&building_data);

    // TODO: load buildings into more comprehensive memory
    
    DataNode ephemerides;
    INFO("Loading Ephemerides")
    if (DataNode::FromFile(&ephemerides, ephemerides_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load datafile %s", ephemerides_path);
    }

    int num_planets = LoadEphemerides(&ephemerides);

    DataNode ship_classes;
    INFO("Loading Ships")
    if (DataNode::FromFile(&ship_classes, ship_classes_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load datafile %s", ship_classes_path);
    }

    int num_ships = LoadShipClasses(&ship_classes);

    DataNode resources_datanode;
    INFO("Loading resources")
    if (DataNode::FromFile(&resources_datanode , resources_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load datafile %s", resources_path);
    }

    LoadResources(&resources_datanode);


    INFO("%d buildings, %d planets, %d ships", num_buildings, num_planets, num_ships)
}

void GlobalState::LoadGame(const char* file_path) {
    DataNode game_data = DataNode();
    if (DataNode::FromFile(&game_data, file_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", file_path);
    }

    Deserialize(&game_data);
}

// Update
void GlobalState::UpdateState(double delta_t) {
    c_transf.HandleInput(delta_t);
    calendar.HandleInput(delta_t);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        // Cancel out of next layer

        // cancel out of focused planet and ship
        if (BuildingConstructionIsOpen()) {
            BuildingConstructionClose();
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
            is_in_pause_menu = !is_in_pause_menu;
            if (is_in_pause_menu) calendar.paused = true;
        }
    }

    active_transfer_plan.Update();
    calendar.AdvanceTime(delta_t);

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

    DrawCircleV(c_transf.TransformV({0}), c_transf.TransformS(GetParentNature()->radius), MAIN_UI_COLOR);
    for (auto [_, planet] : planet_view.each()) {
        planet.Draw(&c_transf);
    }
    for (auto [_, ship] : ship_view.each()) {
        ship.Draw(&c_transf);
    }

    active_transfer_plan.Draw(&c_transf);

    // UI
    UIStart();
    calendar.DrawUI();
    char capital_str[14];
    sprintf(capital_str, "%6d.%3d M$", capital / (int)1e6, capital % 1000000 / 1000);
    DrawTextAligned(capital_str, {GetScreenWidth() / 2.0f, 10}, TEXT_ALIGNMENT_HCENTER & TEXT_ALIGNMENT_TOP, MAIN_UI_COLOR);

    // 
    for (auto [_, planet] : planet_view.each()) {
        if (active_transfer_plan.IsActive()){
            if (active_transfer_plan.plan->departure_planet == planet.id) {
                planet.DrawUI(&c_transf, true, ResourceTransferInvert(active_transfer_plan.plan->resource_transfer), active_transfer_plan.plan->fuel_mass);
            }
            if (active_transfer_plan.plan->arrival_planet == planet.id) {
                planet.DrawUI(&c_transf, false, active_transfer_plan.plan->resource_transfer, -1);
            }
        }
        else if (planet.mouse_hover || planet.id == focused_planet) {
            planet.DrawUI(&c_transf, true, EMPTY_TRANSFER, -1);
        }
    }
    BuildingConstructionUI();
    for (auto [_, ship] : ship_view.each()) {
        ship.DrawUI(&c_transf);
    }
    active_transfer_plan.DrawUI();

    if (is_in_pause_menu){
        _PauseMenu();
    }
    UIEnd();

    DebugFlushText();
    //DrawFPS(0, 0);
}

void GlobalState::Serialize(DataNode* data) const {
    // TODO: refer to the ephemerides used
    
    c_transf.Serialize(data->SetChild("coordinate_transform", DataNode()));
    calendar.Serialize(data->SetChild("calendar", DataNode()));
    data->SetF("capital", capital);
    // ignore transferplanui for now
    data->SetI("focused_planet", (int) focused_planet);
    data->SetI("focused_ship", (int) focused_ship);

    auto planet_view = registry.view<Planet>();
    data->SetArrayChild("planets", planet_view.size());
    int i=0;
    for (auto [entity, planet] : planet_view.each()) {
        DataNode dn2 = DataNode();
        dn2.SetI("id", (int) entity);
        planet.Serialize(data->SetArrayElemChild("planets", i++, dn2));
    }

    auto ship_view = registry.view<Ship>();
    data->SetArrayChild("ships", ship_view.size());
    i=0;
    for (auto [entity, ship] : ship_view.each()) {
        DataNode dn2 = DataNode();
        dn2.SetI("id", (int) entity);
        if (ship.is_parked) {
            dn2.Set("planet", GetPlanet(ship.parent_planet).name);
        }
        ship.Serialize(data->SetArrayElemChild("ships", i++, dn2));
    }
}

bool GlobalState::CompleteTransaction(int delta, const char *message) {
    capital += delta;
    return true;
}

entity_id_t _AddPlanet(GlobalState* gs, const DataNode* data, entity_id_t planet_entity) {
    //entity_map.insert({uuid, planet_entity});
    Planet &planet = gs->registry.emplace<Planet>(planet_entity);
    planet.Deserialize(data);
    planet.id = planet_entity;
    planet.Update();

    return planet_entity;
}

entity_id_t _AddShip(GlobalState* gs, const DataNode* data, entity_id_t ship_entity) {
    //printf("Adding Ship N°%d\n", index)
    //entity_map.insert({uuid, ship_entity});
    Ship &ship = gs->registry.emplace<Ship>(ship_entity);
    
    ship.Deserialize(data);
    if (ship.is_parked) {
        const char* planet_name = data->Get("planet", "NO NAME SPECIFIED");
        const Planet* planet = GetPlanetByName(planet_name);
        if (planet == NULL) {
            FAIL("Error while initializing ship '%s': no such planet '%s'", ship.name, planet_name)
        }
        ship.parent_planet = planet->id;
    }

    ship.id = ship_entity;
    ship.Update();
    return ship_entity;
}

void GlobalState::Deserialize(const DataNode* data) {
    if (data->Has("coordinate_transform")) {
        c_transf.Deserialize(data->GetChild("coordinate_transform"));
    } else {
        c_transf.Make();
    }
    if (data->Has("calendar")) {
        calendar.Deserialize(data->GetChild("calendar"));
    } else {
        calendar.Make(Time(data->GetF("start_time", 0, true)));
    }
    // ignore transferplanui for now

    //LoadEphemeridesFromFile("resources/data/ephemerides.yaml");  // if necaissary
    
    capital = data->GetF("capital", capital);

    // Be explicit to not kill any non-intended things
    auto deletion_view_pl = registry.view<Planet>();
    auto deletion_view_ship = registry.view<Ship>();
    registry.destroy(deletion_view_pl.begin(), deletion_view_pl.end());
    registry.destroy(deletion_view_ship.begin(), deletion_view_ship.end());

    focused_planet = (entity_id_t) data->GetI("focused_planet", -1, true);
    focused_ship = (entity_id_t) data->GetI("focused_ship", -1, true);

    for (int i=0; i < data->GetArrayChildLen("planets"); i++) {
        DataNode* planet_data = data->GetArrayChild("planets", i);
        entity_id_t id;
        if (planet_data->Has("id")) {
            entity_id_t id_hint = (entity_id_t) planet_data->GetI("id");
            id = registry.create(id_hint);
            if (id != id_hint){
                WARNING(
                    "Could not initialize planet %s with intended ID %d. Instead assigned ID %d. This might lead to inconsistencies", 
                    planet_data->Get("name", "UNNAMED"),
                    id_hint, id
                )
            }
        } else {
            id = registry.create();
        }
        _AddPlanet(this, planet_data, id);
    }
    for (int i=0; i < data->GetArrayChildLen("ships"); i++) {
        DataNode* ship_data = data->GetArrayChild("ships", i);

        entity_id_t id;
        if (ship_data->Has("id")) {
            entity_id_t id_hint = (entity_id_t) ship_data->GetI("id");
            id = registry.create(id_hint);
            if (id != id_hint){
                WARNING(
                    "Could not initialize ship %s with intended ID %d. Instead assigned ID %d. This might lead to inconsistencies", 
                    ship_data->Get("name", "UNNAMED"),
                    id_hint, id
                )
            }
        } else {
            id = registry.create();
        }
        _AddShip(this, ship_data, id);
    }
}