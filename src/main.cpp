#include "datanode.hpp"
#include "app_meta.hpp"
#include "audio_server.hpp"
#include "global_state.hpp"

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
    /*RID planet_1 = GetPlanets()->GetIdByName("Tethys");
    RID planet_2 = GetPlanets()->GetIdByName("Titan");
    double dv1, dv2;
    HohmannTransfer(&GetPlanet(planet_1)->orbit, &GetPlanet(planet_2)->orbit, 0, NULL, NULL, &dv1, &dv2);
    INFO("dv1: %f, dv2: %f", dv1, dv2)
    double dv1_true = GetPlanet(planet_1)->GetDVFromExcessVelocity({0, dv1});
    double dv2_true = GetPlanet(planet_2)->GetDVFromExcessVelocity({0, dv2});
    INFO("dv1': %f, dv2': %f", dv1_true, dv2_true)*/

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

    GlobalState* app = GetGlobalState();
    //GetAudioServer()->StartMusic();

    app->Make(1e6);
    app->LoadData();
    //app->LoadGame("resources/data/start_saves/combat_waves.yaml");
    app->LoadGame("resources/data/start_saves/industry.yaml");
    TestingSetup(app);

    // Interpreting cmdline
}

void MainLoopStep(GlobalState* app) {
    app->UpdateState(1./60.);

    BeginDrawing();
    app->render_server.Draw();
    EndDrawing();
}

int main(int argc, const char** argv) {
    if (GetCmdSetting(argc, argv, "-run_tests")) {
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
