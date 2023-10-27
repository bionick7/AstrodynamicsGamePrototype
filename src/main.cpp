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

    GlobalGetState()->Make(1e6);
    GlobalGetState()->Load("irrelevant for now");

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