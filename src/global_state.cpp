#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "logging.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "audio_server.hpp"
#include "timeline.hpp"
#include "debug_console.hpp"
#include "diverse_ui.hpp"
#include "event_popup.hpp"

GlobalState global_state;

void GlobalState::Make(timemath::Time p_time) {
    ui.UIInit();
    calendar.Make(p_time);
    active_transfer_plan.Reset();
    camera.Make();
    focused_planet = GetInvalidId();
    focused_ship = GetInvalidId();
}

void GlobalState::LoadData() {
    string_identifiers.Clear();

    #define NUM 6
    const char* loading_paths[NUM] = {
        "resources/data/ephemeris.yaml",
        "resources/data/resources.yaml",
        "resources/data/ship_modules.yaml",
        "resources/data/ship_classes.yaml",
        "resources/data/techtree.yaml",
        "resources/data/events.yaml"
    };

    int (*load_funcs[NUM])(const DataNode*) = { 
        LoadEphemeris,
        LoadResources,
        LoadShipModules,
        LoadShipClasses,
        LoadTechTree,
        LoadEvents,
    };

    const char* declarations[NUM] {
        "Planets",
        "Resources",
        "ShipModules",
        "ShipClasses",
        "TechTree Nodes",
        "Events",
    };

    int amounts[NUM];
    
    for (int i=0; i < NUM; i++) {
        INFO("Loading %s", declarations[i])
        const DataNode* data = assets::GetData(loading_paths[i]);
        if (data == NULL) {
            FAIL("Could not load resource %s", loading_paths[i]);
        }

        amounts[i] = load_funcs[i](data);
    }

    for (int i=0; i < NUM; i++) {
        INFO("%d %s%s", amounts[i], declarations[i], i == NUM-1 ? "\n" : "; ");
    }

    // INFO(GetModuleByRID(GetGlobalState()->GetFromStringIdentifier("shpmod_water_extractor"))->id)
    // INFO("%f", GetModuleByRID(GetGlobalState()->GetFromStringIdentifier("shpmod_heatshield"))->mass)

    // Load static shaders

    #undef NUM
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
    double min_pixel_distance = 15;  // Threshold
    for (int planet_id = 0; planet_id < gs->planets.GetPlanetCount(); planet_id++) {
        Planet* planet = GetPlanetByIndex(planet_id);
        planet->Update();
        planet->mouse_hover = false;
        double pixel_distance = planet->GetMousePixelDistance();
        if (!ship_selected && pixel_distance < min_pixel_distance) {
            min_pixel_distance = pixel_distance;
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
        //HandleButtonSound(button_state_flags::JUST_HOVER_IN);
    }
    if (IsIdValid(prev_hover) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        HandleButtonSound(button_state_flags::JUST_PRESSED);
    }
    prev_hover = gs->hover;
}

// Update
void GlobalState::UpdateState(double delta_t) {
    calendar.Update(delta_t);
    audio_server.Update(delta_t);
    
    panel_management::HandleDeselect(this);

    active_transfer_plan.Update();
    techtree.Update();
    _UpdateShipsPlanets(this);

    camera.HandleInput();
    global_logic.Update();

    if (frame_count == 0 || IsKeyPressed(KEY_Q)) {
        static const char lorem[] = 
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean nec blandit ante."
            "Nam in mi nibh. Donec nec urna ac ligula faucibus malesuada eget rhoncus eros."
            "Duis id elit vitae neque blandit blandit mollis vel nisl. Vivamus in elit vitae"
            "arcu dapibus lacinia. Mauris efficitur sollicitudin felis, non ultrices arcu fermentum non."
            "Phasellus cursus nunc sem, vitae feugiat mi blandit sit amet. Integer vel sapien diam."
            "Vivamus venenatis et metus sit amet malesuada. Etiam vitae vulputate turpis."
            "Nulla volutpat.";

        Popup* popup = event_popup::AddPopup(600, 600, 300);
        strcpy(popup->title, "Test Popup");
        strcpy(popup->description, "This is a popup, created for the purpose of testing. No further action required\n\n");
        strcpy(popup->description + 81, lorem);
        EmbeddedScene* scene;
        popup->embedded_scene = GetRenderServer()->embedded_scenes.Allocate(&scene);
        scene->Make(5, popup->width, popup->face_height);

        scene->camera_distance = 40;
        scene->meshes[0] = assets::GetWireframe("resources/meshes/ships3D/shp_light_transport.obj");
        scene->meshes[1] = assets::GetWireframe("resources/meshes/ships3D/shp_bulk_transport.obj");
        scene->meshes[2] = assets::GetWireframe("resources/meshes/ships3D/shp_cruiser.obj");
        scene->meshes[3] = assets::GetWireframe("resources/meshes/ships3D/shp_express.obj");
        scene->meshes[4] = assets::GetWireframe("resources/meshes/ships3D/shp_spacestation.obj");
        scene->transforms[0] = MatrixMultiply(MatrixTranslate(20, 0, 0), MatrixRotateY(DEG2RAD *   0));
        scene->transforms[1] = MatrixMultiply(MatrixTranslate(20, 0, 0), MatrixRotateY(DEG2RAD *  90));
        scene->transforms[2] = MatrixMultiply(MatrixTranslate(20, 0, 0), MatrixRotateY(DEG2RAD * 180));
        scene->transforms[3] = MatrixMultiply(MatrixTranslate(20, 0, 0), MatrixRotateY(DEG2RAD * 270));
        scene->transforms[4] = MatrixIdentity();
    }

    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_F5)) {
        assets::Reload();
    }

}

// Draw - Called FROM render_server
void GlobalState::DrawUI() {        
    // UI
    ui.UIStart();
    calendar.DrawUI();
    
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

    ship_modules.UpdateDragging();
    panel_management::DrawUIPanels(this);
    DrawDebugConsole();
    DrawPauseMenu();

    event_popup::UpdateAllPopups();

    ui.UIEnd();

    DebugFlushText();

    // FPS
    if (GetSettingBool("show_fps", false)) {
        DrawText(TextFormat("%2.5f ms", GetFrameTime() * 1000), 0, 0, 20, LIME);
        DrawFPS(0, 20);
    }
}

bool GlobalState::IsKeyBoardFocused() const {
    return IsInDebugConsole();
}

void GlobalState::AddStringIdentifier(TableKey string_id, RID rid) {
    int find = string_identifiers.Find(string_id);
    if (find < 0) {
        int index = string_identifiers.Insert(string_id, rid);
    } else {
        string_identifiers.data[find] = rid;
    }
}

RID GlobalState::GetFromStringIdentifier(TableKey string_id) {
    int find = string_identifiers.Find(string_id);
    if (find < 0) {
        return GetInvalidId();
    }
    return string_identifiers.data[find];
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
    game_data.WriteToFile(file_path, file_format::YAML);
}

void GlobalState::Serialize(DataNode* data) const {
    // TODO: refer to the ephemeris used
    
    // Serialize Camera
    camera.Serialize(data->SetChild("camera"));


    calendar.Serialize(data->SetChild("calendar"));
    techtree.Serialize(data->SetChild("tech"));
    global_vars::Serialize(data->SetChild("global_vars"));

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
            dn2->Set("planet", planets.GetPlanet(ship->GetParentPlanet())->name.GetChar());
        }
        ship->Serialize(dn2);
    }
}

void GlobalState::Deserialize(const DataNode* data) {
    if (data->HasChild("camera")) {
        camera.Deserialize(data->GetChild("camera"));
    } else {
        camera.Make();
    }
    if (data->HasChild("calendar")) {
        calendar.Deserialize(data->GetChild("calendar"));
    } else {
        calendar.Make(timemath::Time(data->GetF("start_time", 0, true)));
    }
    if (data->HasChild("tech")) {
        techtree.Deserialize(data->GetChild("tech"));
    }
    if (data->HasChild("global_vars")) {
        global_vars::Deserialize(data->GetChild("global_vars"));
    }
    // ignore transferplanui for now

    ships.Clear();
    planets.Clear();

    DataNode ephem_data;
    const char* ephemerides_path = "resources/data/ephemeris.yaml";
    if (DataNode::FromFile(&ephem_data, ephemerides_path, file_format::YAML, true) != 0) {
        FAIL("Could not load save %s", ephemerides_path);
    }
    planets.LoadEphemeris(&ephem_data);  // if necessary

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
TechTree*            GetTechTree()            { return &global_state.techtree;              }

// UI elements
TransferPlanUI*      GetTransferPlanUI()      { return &global_state.active_transfer_plan;  }
BattleLog*           GetBattleLog()           { return &global_state.last_battle_log;       }

// servers
AudioServer*         GetAudioServer()         { return &global_state.audio_server;          }
RenderServer*        GetRenderServer()        { return &global_state.render_server;         }
UIGlobals*           GetUI()                  { return &global_state.ui;                    }
GlobalLogic*         GetGlobalLogic()         { return &global_state.global_logic;          }
