//#define RAYMATH_IMPLEMENTATION
#include "global_state.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window title");
    SetTargetFPS(60);
    BasicInit();

    GlobalStateMake(GlobalGetState(), 0uL);
    LoadGlobalState(GlobalGetState(), "irrelevant for now");

    while (!WindowShouldClose()) {  
        UpdateState(GlobalGetState(), 1./60.);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawState(GlobalGetState());
        EndDrawing();
    }

    DestroyGlobalState(GlobalGetState());
    CloseWindow();                  // Close window and OpenGL context

    return 0;
}
