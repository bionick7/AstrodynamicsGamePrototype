#include "datanode.hpp"
#include "app_meta.hpp"
#include "global_state.hpp"
#include "debug_console.hpp"

#include "constants.hpp"
#include "tests.hpp"

const char* GetCmdSetting(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return i == argc - 1 ? "" : argv[i+1];
        }
    }
    return NULL;
}

bool GetCmdSettingBool(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return true;
        }
    }
    return false;
}

void TestingSetup(GlobalState* app) {
    StringBuilder sb;
    GetDVTable(&sb, true);
    sb.WriteToFile("DVTable.txt");
}

void Load(int argc, const char** argv) {
    INFO("Init from working directory: '%s'", GetWorkingDirectory());
    if (GetCmdSettingBool(argc, argv, "-randseed")) {
        SetRandomSeed(0);  // For consistency
    }
    if (GetCmdSettingBool(argc, argv, "-bake")) {
        assets::BakeAllResources();
    }
    if (GetCmdSettingBool(argc, argv, "-unbake")) {
        assets::UnBakeAllResources();
    }
    if (GetCmdSettingBool(argc, argv, "-export_palette")) {
        Palette::ExportToFile("platte.gpl");
    }

    GlobalState* app = GetGlobalState();
    //GetAudioServer()->StartMusic();

    app->Make(1e6);
    app->LoadData();
    //app->LoadGame("resources/data/start_saves/combat_waves.yaml");
    app->LoadGame(TextFormat("resources/data/start_saves/%s.yaml", GetSettingStr("default_savefile")));
    TestingSetup(app);
}

void MainLoopStep(GlobalState* app) {
    app->UpdateState(GetFrameTime());

    BeginDrawing();
    app->render_server.Draw();
    EndDrawing();

    app->frame_count++;  // Only update at the very end
}

int main(int argc, const char** argv) {
    if (GetCmdSetting(argc, argv, "-run_tests")) {
        return UnitTests();
    }

    AppMetaInit();
    Load(argc, argv);

    GlobalState* app = GetGlobalState();
    while (!WindowShouldClose()) {
        AppMetaStep();
        MainLoopStep(app);
    }

    AppMetaClose();    
    return 0;
}
