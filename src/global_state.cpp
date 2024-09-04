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
    ui::PushInset(DEFAULT_FONT_SIZE+4);
    ui::Enclose();
    ui::Write(label);
    button_state_flags::T button_state = ui::AsButton();
    HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
    ui::Pop();
    return button_state & button_state_flags::JUST_PRESSED;
}

bool is_in_pause_menu;
void _PauseMenu() {
    const int menu_width = 200;
    const int button_height = DEFAULT_FONT_SIZE+10;
    const int menu_height = button_height * 3 + 8;
    ui::CreateNew(
        (GetScreenWidth() - menu_width)/2, 
        (GetScreenHeight() - menu_height)/2, 
        menu_width,
        menu_height,
        DEFAULT_FONT_SIZE,
        Palette::ui_main,
        Palette::bg,
        true
    );
    ui::Enclose();
    if (_PauseMenuButton("Save")) {
        INFO("Save")
        DataNode dn;
        GetGlobalState()->Serialize(&dn);
        dn.WriteToFile("save.yaml", FileFormat::YAML);
    }
    if (_PauseMenuButton("Load")) {
        INFO("Load")
        GetGlobalState()->LoadGame("save.yaml");
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
    string_identifiers.clear();

    wren_interface.MakeVM();
    wren_interface.LoadWrenQuests();

    #define NUM 5
    const char* loading_paths[NUM] = {
        "resources/data/ephemeris.yaml",
        "resources/data/resources.yaml",
        "resources/data/ship_modules.yaml",
        "resources/data/ship_classes.yaml",
        "resources/data/techtree.yaml",
    };

    int (*load_funcs[NUM])(const DataNode*) = { 
        LoadEphemeris,
        LoadResources,
        LoadShipModules,
        LoadShipClasses,
        LoadTechTree,
    };

    const char* declarations[NUM] {
        "Planets",
        "Resources",
        "ShipModules",
        "ShipClasses",
        "TechTree Nodes",
    };

    int ammounts[NUM];
    
    for (int i=0; i < NUM; i++) {
        INFO("Loading %s", declarations[i])
        const DataNode* data = assets::GetData(loading_paths[i]);
        if (data == NULL) {
            FAIL("Could not load resource %s", loading_paths[i]);
        }

        ammounts[i] = load_funcs[i](data);
    }

    for (int i=0; i < NUM; i++) {
        INFO("%d %s%s", ammounts[i], declarations[i], i == NUM-1 ? "\n" : "; ");
    }

    // INFO(GetModuleByRID(GetGlobalState()->GetFromStringIdentifier("shpmod_water_extractor"))->id)
    // INFO("%f", GetModuleByRID(GetGlobalState()->GetFromStringIdentifier("shpmod_heatshield"))->mass)

    // Load static shaders

    #undef NUM
}

GlobalState::FocusablesPanels _GetCurrentFocus(GlobalState* gs) {

    // out of techtree
    if (gs->techtree.shown) {
        return GlobalState::TECHTREE;
    }

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
    case GlobalState::TECHTREE:{
        gs->techtree.shown = false;
        break;}
    default: NOT_REACHABLE;
    }
}

RID prev_hover = GetInvalidId();
void _UpdateShipsPlanets(GlobalState* gs) {
    gs->hover = GetInvalidId();
    double min_distance = INFINITY;

    // Ships take priority in selection
    for (auto it = gs->ships.alloc.GetIter(); it; it++) {
        Ship* ship = gs->ships.alloc[it];
        ship->mouse_hover = false;
        ship->Update();
        if (ship->HasMouseHover(&min_distance)) {
            gs->hover = ship->id;
        }
    }
    bool ship_selected = IsIdValidTyped(gs->hover, EntityType::SHIP);
    for (int planet_id = 0; planet_id < gs->planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanetByIndex(planet_id);
        planet->Update();
        planet->mouse_hover = false;
        if (!ship_selected && planet->HasMouseHover(&min_distance)) {
            gs->hover = planet->id;
        }
    }
    
    if (prev_hover != gs->hover && GetUI()->IsPointBlocked(GetMousePosition(), 0) && IsIdValid(gs->hover)) {
        gs->hover = prev_hover;
    }

    switch (IdGetType(gs->hover)) {
    case EntityType::PLANET: GetPlanet(gs->hover)->mouse_hover = true; break;
    case EntityType::SHIP: gs->ships.GetShip(gs->hover)->mouse_hover = true; break;
    default: break;
    }

    if (prev_hover != gs->hover) {
        HandleButtonSound(button_state_flags::JUST_HOVER_IN);
    }
    if (IsIdValid(prev_hover) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        HandleButtonSound(button_state_flags::JUST_PRESSED);
    }
    prev_hover = gs->hover;
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
    quest_manager.Update();
    techtree.Update();
    _UpdateShipsPlanets(this);

    camera.HandleInput();

    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_F5)) {
        assets::Reload();
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
    sprintf(capital_str, "M§M %6" LONG_STRID "d.%3" LONG_STRID "d .mil", 
        factions.GetMoney(factions.player_faction) / (int)1e6, 
        factions.GetMoney(factions.player_faction) % 1000000 / 1000
    );
    DrawTextAligned(
        capital_str, {GetScreenWidth() / 2.0f, 10}, 
        text_alignment::HCENTER & text_alignment::TOP, 
        Palette::ui_main, Palette::bg, 0
    );

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
    //active_transfer_plan.DrawUI();
    ship_modules.UpdateDragging();
    DrawTimeline();
    quest_manager.Draw();
    last_battle_log.DrawUI();
    techtree.DrawUI();
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

const char *GlobalState::AddStringIdentifier(const char *string_id, RID rid) {
    auto pair = string_identifiers.insert_or_assign(std::string(string_id), rid);
    return pair.first->first.c_str();  // points to string in dictionary
}

RID GlobalState::GetFromStringIdentifier(const char *string_id) {
    auto find = string_identifiers.find(std::string(string_id));
    if (find == string_identifiers.end()) {
        return GetInvalidId();
    }
    return find->second;
}

void GlobalState::LoadGame(const char* file_path) {
    if (!assets::HasTextResource(file_path)) {
        ERROR("Could not load save %s", file_path);
        return;
    }
    const DataNode* game_data = assets::GetData(file_path);
    if (game_data->GetChildArrayCount() == 0) {
        ERROR("Could not load save %s", file_path);
        return;
    }

    Deserialize(game_data);
}

void GlobalState::SaveGame(const char* file_path) const {
    DataNode game_data = DataNode();

    Serialize(&game_data);
    game_data.WriteToFile("save.yaml", FileFormat::YAML);
}

void GlobalState::Serialize(DataNode* data) const {
    // TODO: refer to the ephemeris used
    
    // Serialize Camera
    camera.Serialize(data->SetChild("camera"));


    calendar.Serialize(data->SetChild("calendar"));
    quest_manager.Serialize(data->SetChild("quests"));

    factions.Serialize(data);
    techtree.Serialize(data);

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
    techtree.Deserialize(data);

    ships.Clear();
    planets.Clear();

    DataNode ephem_data;
    const char* ephemerides_path = "resources/data/ephemeris.yaml";
    if (DataNode::FromFile(&ephem_data, ephemerides_path, FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", ephemerides_path);
    }
    planets.LoadEphemeris(&ephem_data);  // if necaissary

    focused_planet = RID(data->GetI("focused_planet", -1, true));
    focused_ship = RID(data->GetI("focused_ship", -1, true));

    for (int i=0; i < data->GetChildArrayLen("planets"); i++) {
        DataNode* planet_data = data->GetChildArrayElem("planets", i);
        if (planet_data != NULL)
            planets.AddPlanet(planet_data);
    }
    for (int i=0; i < data->GetChildArrayLen("ships"); i++) {
        DataNode* ship_data = data->GetChildArrayElem("ships", i);
        if (ship_data != NULL)
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
TechTree*            GetTechTree()            { return &global_state.techtree;              }

// UI elements
TransferPlanUI*      GetTransferPlanUI()      { return &global_state.active_transfer_plan;  }
BattleLog*           GetBattleLog()           { return &global_state.last_battle_log;       }

// servers
AudioServer*         GetAudioServer()         { return &global_state.audio_server;          }
WrenInterface*       GetWrenInterface()       { return &global_state.wren_interface;        }
RenderServer*        GetRenderServer()        { return &global_state.render_server;         }
UIGlobals*           GetUI()                  { return &global_state.ui;                    }
