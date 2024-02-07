#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "quest.hpp"
#include "timeline.hpp"
#include "debug_console.hpp"

GlobalState global_state;

void GlobalState::_InspectState() {

}

bool _PauseMenuButton(const char* label) {
    ui::PushInset(0, DEFAULT_FONT_SIZE+4);
    ui::Enclose(Palette::bg, Palette::ui_main);
    ui::Write(label);
    ButtonStateFlags::T button_state = ui::AsButton();
    HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
    ui::Pop();
    return button_state & ButtonStateFlags::JUST_PRESSED;
}

bool is_in_pause_menu;
void _PauseMenu() {
    const int menu_width = 200;
    const int button_height = DEFAULT_FONT_SIZE+4;
    const int menu_height = button_height * 3;
    ui::CreateNew(
        (GetScreenWidth() - menu_width)/2, 
        (GetScreenHeight() - menu_height)/2, 
        menu_width,
        menu_height,
        DEFAULT_FONT_SIZE,
        Palette::ui_main
    );
    ui::Enclose(Palette::bg, Palette::ui_main);
    if (_PauseMenuButton("Save")) {
        INFO("Save")
        DataNode dn;
        GetGlobalState()->Serialize(&dn);
        dn.WriteToFile("save.yaml", FileFormat::YAML);
    }
    if (_PauseMenuButton("Load")) {
        INFO("Load")
        DataNode dn;
        DataNode::FromFile(&dn, "save.yaml", FileFormat::YAML, true, false);
        GetGlobalState()->Deserialize(&dn);
    }
    if (_PauseMenuButton("Exit")) {
        exit(0);
    }
}

void GlobalState::Make(timemath::Time p_time) {
    ui.UIInit();
    calendar.Make(p_time);
    active_transfer_plan.Reset();
    camera.Make();
    focused_planet = GetInvalidId();
    focused_ship = GetInvalidId();
}

void GlobalState::LoadData() {
    wren_interface.MakeVM();
    wren_interface.LoadWrenQuests();

    #define NUM 4
    const char* loading_paths[NUM] = {
        "resources/data/shipmodules.yaml",
        "resources/data/ephemerides.yaml",
        "resources/data/ship_classes.yaml",
        "resources/data/resources.yaml",
    };

    int (*load_funcs[NUM])(const DataNode*) = { 
        LoadShipModules,
        LoadEphemerides,
        LoadShipClasses,
        LoadResources,
    };

    const char* declarations[NUM] {
        "ShipModules",
        "Planets",
        "ShipClasses",
        "Resources",
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

    // INFO(GetModuleByRID(GetModuleRIDFromStringId("shpmod_water_extractor"))->id)
    // INFO("%f", GetModuleByRID(GetModuleRIDFromStringId("shpmod_heatshield"))->mass)

    // Load static shaders

    #undef NUM
}

void GlobalState::LoadGame(const char* file_path) {
    DataNode game_data = DataNode();
    if (DataNode::FromFile(&game_data, file_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", file_path);
    }

    Deserialize(&game_data);
}

GlobalState::FocusablesPanels _GetCurrentFocus(GlobalState* gs) {
    // out of combat_log
    if (gs->last_battle_log.shown) {
        return GlobalState::COMBAT_LOG;
    }

    // out of quest menu
    if (gs->quest_manager.show_ui) {
        return GlobalState::QUEST_MANAGER;
    }

    // out of timeline
    if (TimelineIsOpen()) {
        return GlobalState::TIMELINE;
    }

    // transfer plan UI
    if (gs->active_transfer_plan.IsActive() || gs->active_transfer_plan.IsSelectingDestination()) {
        return GlobalState::TRANSFER_PLAN_UI;
    } 
    
    // cancel out of focused planet and ship
    if (IsIdValid(gs->focused_planet) || IsIdValid(gs->focused_ship)) {
        return GlobalState::PLANET_SHIP_DETAILS;
    } 
    return GlobalState::MAP;
}

void _HandleDeselect(GlobalState* gs) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !IsKeyPressed(KEY_ESCAPE)) {
        return;
    }

    // Cancel out of next layer
    PlaySFX(SFX_CANCEL);

    switch (gs->current_focus) {
    case GlobalState::QUEST_MANAGER:{
        gs->quest_manager.show_ui = false;
        break;}
    case GlobalState::TIMELINE:{
        TimelineClose();
        break;}
    case GlobalState::TRANSFER_PLAN_UI:{
        gs->active_transfer_plan.Abort();
        break;}
    case GlobalState::PLANET_SHIP_DETAILS:{
        gs->focused_planet = GetInvalidId();
        gs->focused_ship = GetInvalidId();
        break;}
    case GlobalState::MAP:{
        is_in_pause_menu = !is_in_pause_menu;
        if (is_in_pause_menu) 
            gs->calendar.paused = true;
        break;}
    case GlobalState::COMBAT_LOG:{
        gs->last_battle_log.shown = false;
        break;}
    default: NOT_REACHABLE;
    }
}

RID prev_hover = GetInvalidId();
void _UpdateShipsPlanets(GlobalState* gs) {
    RID hover = GetInvalidId();
    double min_distance = INFINITY;

    IDList ship_list;
    for (int planet_id = 0; planet_id < gs->planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanetByIndex(planet_id);
        planet->Update();
        planet->mouse_hover = false;
        if (planet->HasMouseHover(&min_distance)) {
            hover = planet->id;
        }
        ship_list.Clear();
        gs->ships.GetOnPlanet(&ship_list, planet->id, 0xFFFFFFFFU);
        for (int i=0; i < ship_list.size; i++) {
            Ship* ship = GetShip(ship_list[i]);
            ship->Update();
            ship->index_on_planet = i;
            ship->total_on_planet = ship_list.size;
            ship->mouse_hover = false;
            if (ship->HasMouseHover(&min_distance)) {
                hover = ship->id;
            }
        }
    }

    // Update "orphan" ships
    gs->ships.GetOnPlanet(&ship_list, GetInvalidId(), 0xFFFFFFFFU);
    for (int i=0; i < ship_list.size; i++) {
        Ship* ship = GetShip(ship_list[i]);
        ship->Update();
        ship->mouse_hover = false;
        if (ship->HasMouseHover(&min_distance)) {
            hover = ship->id;
        }
    }

    switch (IdGetType(hover)) {
    case EntityType::PLANET: GetPlanet(hover)->mouse_hover = true; break;
    case EntityType::SHIP: gs->ships.GetShip(hover)->mouse_hover = true; break;
    default: break;
    }

    if (prev_hover != hover) {
    //if (!IsIdValid(prev_hover.id) && IsIdValid(hover.id)) {
        HandleButtonSound(ButtonStateFlags::JUST_HOVER_IN);
    }
    if (IsIdValid(prev_hover) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        HandleButtonSound(ButtonStateFlags::JUST_PRESSED);
    }
    prev_hover = hover;
}

// Update
void GlobalState::UpdateState(double delta_t) {
    calendar.HandleInput(delta_t);
    audio_server.Update(delta_t);
    
    current_focus = _GetCurrentFocus(this);
    _HandleDeselect(this);
    calendar.AdvanceTime(delta_t);
    active_transfer_plan.Update();
    wren_interface.Update();
    quest_manager.Update(delta_t);
    _UpdateShipsPlanets(this);

    camera.HandleInput();

    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_F5)) {
        ReloadShaders();
        RenderServer::ReloadShaders();
    }

    // AI update
    factions.Update();
}

// Draw - Called FROM render_server
void GlobalState::DrawUI() {        
    // UI
    ui.UIStart();
    calendar.DrawUI();
    
    // Money counter
    char capital_str[21];
    sprintf(capital_str, "M§M %6" LONG_STRID ".%3" LONG_STRID " .mil", 
        factions.GetMoney(factions.player_faction) / (int)1e6, 
        factions.GetMoney(factions.player_faction) % 1000000 / 1000
    );
    DrawTextAligned(capital_str, {GetScreenWidth() / 2.0f, 10}, TextAlignment::HCENTER & TextAlignment::TOP, Palette::ui_main);

    // planets
    for (int planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanetByIndex(planet_id);
        planet->DrawUI();
    }
    // ships
    for (auto it = ships.alloc.GetIter(); it; it++) {
        Ship* ship = ships.alloc[it];
        ship->DrawUI();
    }
    // Standalone UI Panels
    active_transfer_plan.DrawUI();
    ship_modules.UpdateDragging();
    DrawTimeline();
    quest_manager.Draw();
    last_battle_log.DrawUI();
    DrawDebugConsole();
    
    if (is_in_pause_menu){
        _PauseMenu();
    }
    ui.UIEnd();

    DebugFlushText();
    if (GetSettingBool("show_fps", false)) {
        DrawText(TextFormat("%2.5f ms", GetFrameTime() * 1000), 0, 0, 20, LIME);
        DrawFPS(0, 20);
    }
}

bool GlobalState::IsKeyBoardFocused() const {
    return IsInDebugConsole();
}

void GlobalState::Serialize(DataNode* data) const {
    // TODO: refer to the ephemerides used
    
    // Serialize Camera
    camera.Serialize(data->SetChild("camera"));


    calendar.Serialize(data->SetChild("calendar"));
    quest_manager.Serialize(data->SetChild("quests"));

    factions.Serialize(data);

    // ignore transferplanui for now
    data->SetI("focused_planet", focused_planet.AsInt());
    data->SetI("focused_ship", focused_ship.AsInt());

    data->CreatChildArray("planets", (int) planets.GetPlanetCount());
    int i=0;
    for (int planet_index = 0; planet_index < planets.GetPlanetCount(); planet_index++) {
        DataNode* dn2 = data->InsertIntoChildArray("planets", i++);
        GetPlanetByIndex(planet_index)->Serialize(dn2);
        dn2->SetI("id", RID(planet_index, EntityType::PLANET).AsInt());
    }

    data->CreatChildArray("ships", ships.alloc.Count());
    i=0;
    for (auto it = ships.alloc.GetIter(); it; it++) {
        Ship* ship = ships.alloc.Get(it);
        DataNode* dn2 = data->InsertIntoChildArray("ships", i++);
        dn2->SetI("id", it.GetId().AsInt());
        if (ship->IsParked()) {
            dn2->Set("planet", planets.GetPlanet(ship->GetParentPlanet())->name);
        }
        ship->Serialize(dn2);
    }
}

void GlobalState::Deserialize(const DataNode* data) {
    if (data->HasChild("coordinate_transform")) {
        camera.Deserialize(data->GetChild("camera"));
    } else {
        camera.Make();
    }
    if (data->HasChild("calendar")) {
        calendar.Deserialize(data->GetChild("calendar"));
    } else {
        calendar.Make(timemath::Time(data->GetF("start_time", 0, true)));
    }
    // ignore transferplanui for now

    factions.Deserialize(data);

    ships.alloc.Clear(); 
    planets = Planets();

    DataNode ephem_data;
    const char* ephemerides_path = "resources/data/ephemerides.yaml";
    if (DataNode::FromFile(&ephem_data, ephemerides_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", ephemerides_path);
    }
    planets.LoadEphemerides(&ephem_data);  // if necaissary

    focused_planet = RID(data->GetI("focused_planet", -1, true));
    focused_ship = RID(data->GetI("focused_ship", -1, true));

    for (int i=0; i < data->GetChildArrayLen("planets"); i++) {
        DataNode* planet_data = data->GetChildArrayElem("planets", i);
        planets.AddPlanet(planet_data);
    }
    for (int i=0; i < data->GetChildArrayLen("ships"); i++) {
        DataNode* ship_data = data->GetChildArrayElem("ships", i);
        ships.AddShip(ship_data);
    }

    // Dependency on planets
    if (data->HasChild("quests")) {
        quest_manager.Deserialize(data->GetChild("quests"));
    } else {
        quest_manager.Make();
    }

    factions.InitializeAI();
}

GlobalState* GetGlobalState() { return &global_state; }
timemath::Time GlobalGetNow() { return global_state.calendar.time; }

// global simple items
GameCamera*          GetCamera()              { return &global_state.camera; }
Calendar*            GetCalendar()            { return &global_state.calendar;              }

// collections
Ships*               GetShips()               { return &global_state.ships;                 }
Planets*             GetPlanets()             { return &global_state.planets;               }
ShipModules*         GetShipModules()         { return &global_state.ship_modules;          }
QuestManager*        GetQuestManager()        { return &global_state.quest_manager;         }
Factions*            GetFactions()            { return &global_state.factions;              }

// UI elements
TransferPlanUI*      GetTransferPlanUI()      { return &global_state.active_transfer_plan;  }
BattleLog*           GetBattleLog()           { return &global_state.last_battle_log;       }

// servers
AudioServer*         GetAudioServer()         { return &global_state.audio_server;          }
WrenInterface*       GetWrenInterface()       { return &global_state.wren_interface;        }
RenderServer*        GetRenderServer()        { return &global_state.render_server;         }
UIGlobals*           GetUI()                  { return &global_state.ui;                    }
