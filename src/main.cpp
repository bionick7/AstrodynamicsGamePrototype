#include "transfer_plan.hpp"
#include "datanode.hpp"
#define RETURN_OR_CONTINUE(fn_call) {int test_result = fn_call; if(test_result != 0) return test_result;}

#include "global_state.hpp"
#include "constants.hpp"
#include "ui.hpp"

const char* WINDOW_TITLE = "Astro navigation game prototype";

const char* GetSetting(int argc, const char** argv, const char* find) {
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], find) == 0) {
            return i == argc - 1 ? "" : argv[i+1];
        }
    }
    return NULL;
}

int main(int argc, const char** argv) {
    // Initializing
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    INFO("Init complete");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
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

    if (GetSetting(argc, argv, "--run_tests")) {
        printf("Running Tests\n");
        //RETURN_OR_CONTINUE(AllocatorTest);
        RETURN_OR_CONTINUE(TransferPlanTests());
        RETURN_OR_CONTINUE(DataNodeTests());
        printf("All tests Sucessfull\n");
        return 0;
    }

    // Main loop
    while (!WindowShouldClose()) {  
        app->UpdateState(1./60.);

        BeginDrawing();
        ClearBackground(BG_COLOR);
        app->DrawState();
        EndDrawing();
    }

    CloseWindow();                  // Close window and OpenGL context

    return 0;
}