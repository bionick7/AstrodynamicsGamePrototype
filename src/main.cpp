//#define RUN_TESTS

#ifdef RUN_TESTS
#include "resource_allocator.hpp"

int main() {
    int allocator_test_result = AllocatorTest();
    if (!allocator_test_result) return allocator_test_result;
}
#else
#include "global_state.hpp"
#include "ui.hpp"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window title");
    SetTargetFPS(60);
    UIInit();

    GlobalStateMake(GlobalGetState(), 1e6);
    LoadGlobalState(GlobalGetState(), "irrelevant for now");

    while (!WindowShouldClose()) {  
        UpdateState(GlobalGetState(), 1./60.);

        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawState(GlobalGetState());
        EndDrawing();
    }

    DestroyGlobalState(GlobalGetState());
    CloseWindow();                  // Close window and OpenGL context

    return 0;
}
#endif  // RUN_TESTS