#include "datanode.hpp"
#include "app_meta.hpp"
#include "audio_server.hpp"
#include "global_state.hpp"

#include "constants.hpp"
#include "tests.hpp"

const char* GetSetting(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return i == argc - 1 ? "" : argv[i+1];
        }
    }
    return NULL;
}

void TestingSetup(GlobalState* app) {
    const WrenQuestTemplate* template_ = GetWrenInterface()->GetWrenQuest("raiders");
    if (template_ != NULL) {
        app->quest_manager.ForceQuest(template_);
    }
}

void Load(int argc, const char** argv) {
    INFO("Init from working directory: '%s'", GetWorkingDirectory());
    if (!GetSetting(argc, argv, "-randseed")) {
        SetRandomSeed(0);  // For consistency
    }

    UIInit();
    GetAudioServer()->LoadSFX("unused string input :)");
    //GetAudioServer()->StartMusic();

    GlobalState* app = GlobalGetState();
    app->Make(1e6);
    app->LoadData();
    app->LoadGame("resources/data/start_saves/combat_waves.yaml");
    TestingSetup(app);

    // Interpreting cmdline
}

void MainLoopStep(GlobalState* app) {
    app->UpdateState(1./60.);

    BeginDrawing();
    ClearBackground(Palette::bg);
    app->DrawState();
    EndDrawing();
}

int main(int argc, const char** argv) {
    if (GetSetting(argc, argv, "-run_tests")) {
        return UnitTests();
    }

    // Initializing
    AppMetaInit();
    Load(argc, argv);

    GlobalState* app = GlobalGetState();

    // Main loop
    while (!WindowShouldClose()) {
        AppMetaStep();
        MainLoopStep(app);
    }

    AppMetaClose();    
    return 0;
}