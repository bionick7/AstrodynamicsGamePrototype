#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "app_meta.hpp"

// For tests
#include "global_state.hpp"
#include "constants.hpp"
#include "ui.hpp"

const char* GetSetting(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return i == argc - 1 ? "" : argv[i+1];
        }
    }
    return NULL;
}

#define RETURN_OR_CONTINUE(fn_call) {int test_result = fn_call; if(test_result != 0) return test_result;}
int UnitTests() {
    printf("Running Tests\n");
    RETURN_OR_CONTINUE(TransferPlanTests());
    RETURN_OR_CONTINUE(DataNodeTests());
    RETURN_OR_CONTINUE(TimeTests());
    printf("All tests Sucessfull\n");
    return 0;
}

void Load(int argc, const char** argv) {
    INFO("Init complete");
    UIInit();

    GlobalState* app = GlobalGetState();
    INFO("cwd: '%s'", GetWorkingDirectory());
    app->Make(1e6);
    app->LoadConfigs(
        "resources/data/ephemerides.yaml",
        "resources/data/modules.yaml"
    );
    app->LoadGame("resources/data/start_state.yaml");

    // Interpreting cmdline
    const char* module_outp_fp = GetSetting(argc, argv, "--module_outp");
    if (module_outp_fp != NULL){
        WriteModulesToFile(module_outp_fp);
    }
}

void MainLoopStep(GlobalState* app) {
    app->UpdateState(1./60.);

    BeginDrawing();
    ClearBackground(BG_COLOR);
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

    CloseWindow();                  // Close window and OpenGL context
    return 0;
}