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
    /*RID planet_1 = GetPlanets()->GetIndexByName("Tethys");
    RID planet_2 = GetPlanets()->GetIndexByName("Titan");
    double dv1, dv2;
    HohmannTransfer(&GetPlanet(planet_1)->orbit, &GetPlanet(planet_2)->orbit, 0, NULL, NULL, &dv1, &dv2);
    INFO("dv1: %f, dv2: %f", dv1, dv2)
    double dv1_true = GetPlanet(planet_1)->GetDVFromExcessVelocity({0, dv1});
    double dv2_true = GetPlanet(planet_2)->GetDVFromExcessVelocity({0, dv2});
    INFO("dv1': %f, dv2': %f", dv1_true, dv2_true)*/

    printf("'%s'", "");

    return;

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

    GlobalState* app = GetGlobalState();
    GetAudioServer()->LoadSFX();
    //GetAudioServer()->StartMusic();

    app->Make(1e6);
    app->LoadData();
    //app->LoadGame("resources/data/start_saves/combat_waves.yaml");
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

    GlobalState* app = GetGlobalState();

    // Main loop
    while (!WindowShouldClose()) {
        AppMetaStep();
        MainLoopStep(app);
    }

    AppMetaClose();    
    return 0;
}
