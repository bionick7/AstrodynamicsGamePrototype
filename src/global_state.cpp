#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "quests.hpp"

GlobalState global_state;

bool IsIdValid(entity_id_t id) {
    if (id == UINT32_MAX) return false;
    //if (!global_state.registry.valid(id)) return false;
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

void GlobalState::_InspectState() {

}

bool _PauseMenuButton(const char* label) {
    UIContextPushInset(0, 20);
    UIContextEnclose(0, 0, BG_COLOR, MAIN_UI_COLOR);
    UIContextWrite(label);
    ButtonStateFlags button_state = UIContextAsButton();
    HandleButtonSound(button_state & BUTTON_STATE_FLAG_JUST_PRESSED);
    UIContextPop();
    return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
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
        exit(0);
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
    #define NUM 5
    const char* loading_paths[NUM] = {
        "resources/data/buildings.yaml",
        "resources/data/ephemerides.yaml",
        "resources/data/ship_classes.yaml",
        "resources/data/resources.yaml",
        "resources/data/quests.yaml"
    };

    int (*load_funcs[NUM])(const DataNode*) = { 
        LoadBuildings,
        LoadEphemerides,
        LoadShipClasses,
        LoadResources,
        LoadQuests
    };

    const char* declarations[NUM] {
        "Buildings",
        "Planets",
        "ShipClasses",
        "Resources",
        "Quests",
    };

    int ammounts[NUM];
    
    for (int i=0; i < NUM; i++) {
        INFO("Loading %s", declarations[i])
        DataNode data;
        if (DataNode::FromFile(&data, loading_paths[i], FileFormat::YAML, true) != 0) {
            FAIL("Could not load save %s", loading_paths[i]);
        }

        ammounts[i] = load_funcs[i](&data);
    }

    for (int i=0; i < NUM; i++) {
        printf("%d %s%s", ammounts[i], declarations[i], i == NUM-1 ? "\n" : "; ");
    }

    #undef NUM
}

void GlobalState::LoadGame(const char* file_path) {
    DataNode game_data = DataNode();
    if (DataNode::FromFile(&game_data, file_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", file_path);
    }

    Deserialize(&game_data);
}


void _HandleDeselect(GlobalState* gs) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !IsKeyPressed(KEY_ESCAPE)) {
        return;
    }

    // Cancel out of next layer
    PlaySFX(SFX_CANCEL);

    // cancel out of focused planet and ship
    if (BuildingConstructionIsOpen()) {
        BuildingConstructionClose();
    } 

    // transfer plan UI
    else if (gs->active_transfer_plan.IsActive() || gs->active_transfer_plan.IsSelectingDestination()) {
        gs->active_transfer_plan.Abort();
    } 
    // cancel out of focused planet and ship
    else if (IsIdValid(gs->focused_planet) || IsIdValid(gs->focused_ship)) {
        gs->focused_planet = GetInvalidId();
        gs->focused_ship = GetInvalidId();
    } else {
        is_in_pause_menu = !is_in_pause_menu;
        if (is_in_pause_menu) gs->calendar.paused = true;
    }
}

Clickable prev_hover {TYPE_NONE, GetInvalidId()};
void _UpdateShipsPlanets(GlobalState* gs) {
    Clickable hover = {TYPE_NONE, GetInvalidId()};
    double min_distance = INFINITY;

    for (int planet_id = 0; planet_id < gs->planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanet((entity_id_t) planet_id);
        planet->Update();
        planet->mouse_hover = false;
        if (planet->HasMouseHover(&min_distance)) {
            hover = {TYPE_PLANET, planet->id};
        }
    }
    int total_ship_count = 0;
    for (auto it = gs->ships.alloc.GetIter(); gs->ships.alloc.IsIterGoing(it); gs->ships.alloc.IncIterator(&it)) {
        Ship* ship = gs->ships.alloc[it];
        ship->Update();
        if (IsIdValid(ship->parent_planet)) {
            ship->index_on_planet = total_ship_count++;
        }
        ship->mouse_hover = false;
        if (ship->HasMouseHover(&min_distance)) {
            hover = {TYPE_SHIP, ship->id};
        }
    }
    switch (hover.type) {
    case TYPE_PLANET: GetPlanet(hover.id)->mouse_hover = true; break;
    case TYPE_SHIP: GetShip(hover.id)->mouse_hover = true; break;
    case TYPE_NONE:
    default: break;
    }

    if (prev_hover.id != hover.id) {
    //if (!IsIdValid(prev_hover.id) && IsIdValid(hover.id)) {
        HandleButtonSound(BUTTON_STATE_FLAG_JUST_HOVER_IN);
    }
    if (IsIdValid(prev_hover.id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        HandleButtonSound(BUTTON_STATE_FLAG_JUST_PRESSED);
    }
    prev_hover = hover;
}

// Update
void GlobalState::UpdateState(double delta_t) {
    c_transf.HandleInput(delta_t);
    calendar.HandleInput(delta_t);
    GetAudioServer()->Update(delta_t);
    //GetQuestManager()->Update(delta_t);

    _HandleDeselect(this);
    active_transfer_plan.Update();
    calendar.AdvanceTime(delta_t);
    _UpdateShipsPlanets(this);
}

// Draw
void GlobalState::DrawState() {

    DrawCircleV(c_transf.TransformV({0}), c_transf.TransformS(planets.GetParentNature()->radius), MAIN_UI_COLOR);
    for (entity_id_t planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        planets.GetPlanet(planet_id)->Draw(&c_transf);
    }
    for (auto it = ships.alloc.GetIter(); ships.alloc.IsIterGoing(it); ships.alloc.IncIterator(&it)) {
        Ship* ship = ships.alloc[it];
        ship->Draw(&c_transf);
    }

    active_transfer_plan.Draw(&c_transf);

    // UI
    UIStart();
    calendar.DrawUI();
    char capital_str[14];
    sprintf(capital_str, "%6d.%3d M$", capital / (int)1e6, capital % 1000000 / 1000);
    DrawTextAligned(capital_str, {GetScreenWidth() / 2.0f, 10}, TEXT_ALIGNMENT_HCENTER & TEXT_ALIGNMENT_TOP, MAIN_UI_COLOR);

    // 
    for (entity_id_t planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        Planet* planet = planets.GetPlanet(planet_id);
        if (active_transfer_plan.IsActive()){
            if (active_transfer_plan.plan->departure_planet == planet->id) {
                planet->DrawUI(&c_transf, true, ResourceTransferInvert(active_transfer_plan.plan->resource_transfer), active_transfer_plan.plan->fuel_mass);
            }
            if (active_transfer_plan.plan->arrival_planet == planet->id) {
                planet->DrawUI(&c_transf, false, active_transfer_plan.plan->resource_transfer, -1);
            }
        }
        else if (planet->mouse_hover || planet->id == focused_planet) {
            planet->DrawUI(&c_transf, true, EMPTY_TRANSFER, -1);
        }
    }
    BuildingConstructionUI();
    for (auto it = ships.alloc.GetIter(); ships.alloc.IsIterGoing(it); ships.alloc.IncIterator(&it)) {
        Ship* ship = ships.alloc[it];
        ship->DrawUI(&c_transf);
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

    data->SetArrayChild("planets", (int) planets.GetPlanetCount());
    int i=0;
    for (entity_id_t planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        DataNode dn2 = DataNode();
        dn2.SetI("id", (int) planet_id);
        planets.GetPlanet(planet_id)->Serialize(data->SetArrayElemChild("planets", i++, dn2));
    }

    data->SetArrayChild("ships", ships.alloc.Count());
    i=0;
    for (auto it = ships.alloc.GetIter(); ships.alloc.IsIterGoing(it); ships.alloc.IncIterator(&it)) {
        Ship* ship = ships.alloc.Get(it);
        DataNode dn2 = DataNode();
        dn2.SetI("id", (int) it.index);
        if (ship->is_parked) {
            dn2.Set("planet", planets.GetPlanet(ship->parent_planet)->name);
        }
        ship->Serialize(data->SetArrayElemChild("ships", i++, dn2));
    }
}

bool GlobalState::CompleteTransaction(int delta, const char *message) {
    capital += delta;
    return true;
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

    
    capital = data->GetF("capital", capital);

    //ships.ClearShips();
    ships.alloc.Clear(); 
    planets = Planets();
    planets.Init(data->GetArrayChildLen("planets"));

    DataNode ephem_data;
    if (DataNode::FromFile(&ephem_data, "resources/data/ephemerides.yaml", FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", "resources/data/ephemerides.yaml");
    }
    planets.LoadEphemerides(&ephem_data);  // if necaissary

    focused_planet = (entity_id_t) data->GetI("focused_planet", -1, true);
    focused_ship = (entity_id_t) data->GetI("focused_ship", -1, true);

    for (int i=0; i < data->GetArrayChildLen("planets"); i++) {
        DataNode* planet_data = data->GetArrayChild("planets", i);
        entity_id_t id = planets.AddPlanet(planet_data);
        //entity_id_t id;
        if (planet_data->Has("id")) {
            ASSERT_EQUAL_INT(id, (entity_id_t) planet_data->GetI("id"))
        }
        //    entity_id_t id_hint = (entity_id_t) planet_data->GetI("id");
        //    id = registry.create(id_hint);
        //    if (id != id_hint){
        //        WARNING(
        //            "Could not initialize planet %s with intended ID %d. Instead assigned ID %d. This might lead to inconsistencies", 
        //            planet_data->Get("name", "UNNAMED"),
        //            id_hint, id
        //        )
        //    }
        //} else {
        //    id = registry.create();
        //}
    }
    for (int i=0; i < data->GetArrayChildLen("ships"); i++) {
        DataNode* ship_data = data->GetArrayChild("ships", i);
        entity_id_t id = ships.AddShip(ship_data);

        if (ship_data->Has("id")) {
            ASSERT_EQUAL_INT(id, (entity_id_t) ship_data->GetI("id"))
        }

        //entity_id_t id;
        //if (ship_data->Has("id")) {
        //    entity_id_t id_hint = (entity_id_t) ship_data->GetI("id");
        //    if (id != id_hint){
        //        WARNING(
        //            "Could not initialize ship %s with intended ID %d. Instead assigned ID %d. This might lead to inconsistencies", 
        //            ship_data->Get("name", "UNNAMED"),
        //            id_hint, id
        //        )
        //    }
        //} else {
        //    id = AddShip(this, ship_data, id);
        //}
    }
}