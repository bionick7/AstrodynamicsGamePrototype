//#define RUN_TESTS

#ifdef RUN_TESTS
//#include "resource_allocator.hpp"
#include "transfer_plan.hpp"
#include "datahandling.hpp"

#define RETURN_OR_CONTINUE(fn_call) {int test_result = fn_call; if(test_result != 0) return test_result;}

int main() {
    printf("Running Tests\n");
    //RETURN_OR_CONTINUE(AllocatorTest);
    RETURN_OR_CONTINUE(TransferPlanTests());
    RETURN_OR_CONTINUE(DataNodeTests());
    printf("All tests Sucessfull\n");
    return 0;
}
#else
#include "global_state.hpp"
#include "ui.hpp"

const char* WINDOW_TITLE = "Astro navigation game prototype";

int main(int argc, const char** argv) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    UIInit();

    printf("cwd: '%s'\n", GetWorkingDirectory());
    GlobalGetState()->Make(1e6);
    GlobalGetState()->Load("resources/data/start_state.yaml");

    while (!WindowShouldClose()) {  
        GlobalGetState()->UpdateState(1./60.);

        BeginDrawing();
        ClearBackground(BG_COLOR);
        GlobalGetState()->DrawState();
        EndDrawing();
    }

    CloseWindow();                  // Close window and OpenGL context

    return 0;
}
#endif  // RUN_TESTS