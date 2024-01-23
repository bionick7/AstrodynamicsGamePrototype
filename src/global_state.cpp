#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "quest.hpp"
#include "timeline.hpp"

GlobalState global_state;

GlobalState* GlobalGetState() {
    return &global_state;
}

timemath::Time GlobalGetNow() {
    return global_state.calendar.time;
}

void GlobalState::_InspectState() {

}

bool _PauseMenuButton(const char* label) {
    UIContextPushInset(0, 20);
    UIContextEnclose(Palette::bg, Palette::ui_main);
    UIContextWrite(label);
    ButtonStateFlags::T button_state = UIContextAsButton();
    HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
    UIContextPop();
    return button_state & ButtonStateFlags::JUST_PRESSED;
}

bool is_in_pause_menu;
void _PauseMenu() {
    const int menu_width = 200;
    const int button_height = 20;
    const int menu_height = button_height * 3;
    UIContextCreateNew(
        (GetScreenWidth() - menu_width)/2, 
        (GetScreenHeight() - menu_height)/2, 
        menu_width,
        menu_height,
        16,
        Palette::ui_main
    );
    UIContextEnclose(Palette::bg, Palette::ui_main);
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

void GlobalState::Make(timemath::Time p_time) {
    ui.UIInit();
    calendar.Make(p_time);
    active_transfer_plan.Reset();
    c_transf.Make();
    focused_planet = GetInvalidId();
    focused_ship = GetInvalidId();
    money = 0;
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
    c_transf.HandleInput(delta_t);
    calendar.HandleInput(delta_t);
    audio_server.Update(delta_t);
    
    current_focus = _GetCurrentFocus(this);
    _HandleDeselect(this);
    calendar.AdvanceTime(delta_t);
    active_transfer_plan.Update();
    wren_interface.Update();
    quest_manager.Update(delta_t);
    _UpdateShipsPlanets(this);
}

// Draw
void GlobalState::DrawState() {
    DrawCircleV(c_transf.TransformV({0}), c_transf.TransformS(planets.GetParentNature()->radius), Palette::ui_main);
    for (int planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        GetPlanetByIndex(planet_id)->Draw(&c_transf);
    }
    for (auto it = ships.alloc.GetIter(); it; it++) {
        Ship* ship = ships.alloc[it];
        ship->Draw(&c_transf);
    }

    active_transfer_plan.Draw(&c_transf);

    // UI
    ui.UIStart();
    calendar.DrawUI();
    char capital_str[21];
    sprintf(capital_str, "M§M %6" LONG_STRID ".%3" LONG_STRID " .mil", money / (int)1e6, money % 1000000 / 1000);
    DrawTextAligned(capital_str, {GetScreenWidth() / 2.0f, 10}, TextAlignment::HCENTER & TextAlignment::TOP, Palette::ui_main);

    // planets
    for (int planet_id = 0; planet_id < planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanetByIndex(planet_id);
        planet->DrawUI();
    }
    for (auto it = ships.alloc.GetIter(); it; it++) {
        Ship* ship = ships.alloc[it];
        ship->DrawUI();
    }
    active_transfer_plan.DrawUI();
    ship_modules.UpdateDragging();
    DrawTimeline();
    quest_manager.Draw();
    last_battle_log.DrawUI();
    
    if (is_in_pause_menu){
        _PauseMenu();
    }
    ui.UIEnd();

    DebugFlushText();
    //DrawFPS(0, 0);
}

void GlobalState::Serialize(DataNode* data) const {
    // TODO: refer to the ephemerides used
    
    c_transf.Serialize(data->SetChild("coordinate_transform", DataNode()));
    calendar.Serialize(data->SetChild("calendar", DataNode()));
    quest_manager.Serialize(data->SetChild("quests", DataNode()));

    data->SetF("capital", money);
    // ignore transferplanui for now
    data->SetI("focused_planet", focused_planet.AsInt());
    data->SetI("focused_ship", focused_ship.AsInt());

    data->SetArrayChild("planets", (int) planets.GetPlanetCount());
    int i=0;
    for (int planet_index = 0; planet_index < planets.GetPlanetCount(); planet_index++) {
        DataNode dn2 = DataNode();
        dn2.SetI("id", RID(planet_index, EntityType::PLANET).AsInt());
        GetPlanetByIndex(planet_index)->Serialize(data->SetArrayElemChild("planets", i++, dn2));
    }

    data->SetArrayChild("ships", ships.alloc.Count());
    i=0;
    for (auto it = ships.alloc.GetIter(); it; it++) {
        Ship* ship = ships.alloc.Get(it);
        DataNode dn2 = DataNode();
        dn2.SetI("id", it.GetId().AsInt());
        if (ship->IsParked()) {
            dn2.Set("planet", planets.GetPlanet(ship->GetParentPlanet())->name);
        }
        ship->Serialize(data->SetArrayElemChild("ships", i++, dn2));
    }
}

// delta is the ammount of capital transferred TO the player
bool GlobalState::CompleteTransaction(int delta, const char *message) {
    money += delta;
    return true;
}

void GlobalState::Deserialize(const DataNode* data) {
    if (data->HasChild("coordinate_transform")) {
        c_transf.Deserialize(data->GetChild("coordinate_transform"));
    } else {
        c_transf.Make();
    }
    if (data->HasChild("calendar")) {
        calendar.Deserialize(data->GetChild("calendar"));
    } else {
        calendar.Make(timemath::Time(data->GetF("start_time", 0, true)));
    }
    // ignore transferplanui for now


    money = data->GetF("capital", money);

    ships.alloc.Clear(); 
    planets = Planets();

    DataNode ephem_data;
    if (DataNode::FromFile(&ephem_data, "resources/data/ephemerides.yaml", FileFormat::YAML, true) != 0) {
        FAIL("Could not load save %s", "resources/data/ephemerides.yaml");
    }
    planets.LoadEphemerides(&ephem_data);  // if necaissary

    focused_planet = RID(data->GetI("focused_planet", -1, true));
    focused_ship = RID(data->GetI("focused_ship", -1, true));

    for (int i=0; i < data->GetArrayChildLen("planets"); i++) {
        DataNode* planet_data = data->GetArrayChild("planets", i);
        planets.AddPlanet(planet_data);
    }
    for (int i=0; i < data->GetArrayChildLen("ships"); i++) {
        DataNode* ship_data = data->GetArrayChild("ships", i);
        ships.AddShip(ship_data);
    }

    // Dependency on planets
    if (data->HasChild("quests")) {
        quest_manager.Deserialize(data->GetChild("quests"));
    } else {
        quest_manager.Make();
    }
}

AudioServer* GetAudioServer() {
    return &global_state.audio_server;
}

WrenInterface* GetWrenInterface() {
    return &global_state.wren_interface;
}

UIGlobals* GlobalUI() {
    return &global_state.ui;
}