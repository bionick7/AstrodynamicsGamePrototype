#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "app_meta.hpp"
#include "audio_server.hpp"

// For tests
#include "global_state.hpp"
#include "constants.hpp"
#include "ui.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"

const char* GetSetting(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return i == argc - 1 ? "" : argv[i+1];
        }
    }
    return NULL;
}

#define RETURN_OR_CONTINUE(fn_call) {\
    INFO("Running '%s'", #fn_call) \
    int test_result = fn_call; \
    if(test_result != 0) return test_result;\
}

int UnitTests() {
    INFO("Running Tests");
    RETURN_OR_CONTINUE(TransferPlanTests());
    RETURN_OR_CONTINUE(DataNodeTests());
    RETURN_OR_CONTINUE(TimeTests());
    RETURN_OR_CONTINUE(IDAllocatorListTests());
    RETURN_OR_CONTINUE(StringBuilderTests());
    INFO("All tests Sucessfull\n");
    return 0;
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
    app->LoadGame("resources/data/start_state.yaml");

    // Interpreting cmdline
    const char* building_outp_fp = GetSetting(argc, argv, "--building_outp");
    if (building_outp_fp != NULL){
        WriteBuildingsToFile(building_outp_fp);
    }
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

    CloseWindow();  // Close window and OpenGL context
    return 0;
}