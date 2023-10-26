//#define RAYMATH_IMPLEMENTATION
#include "global_state.h"
#include "ui.h"

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
