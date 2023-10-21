//#define RAYMATH_IMPLEMENTATION
#include "global_state.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "geam");
    SetTargetFPS(60);

    MakeGlobalState(GetGlobalState(), 0uL);
    LoadGlobalState(GetGlobalState(), "irrelevant for now");

    while (!WindowShouldClose()) {  
        UpdateState(GetGlobalState(), 1./60.);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawState(GetGlobalState());
        EndDrawing();
    }

    DestroyGlobalState(GetGlobalState());
    CloseWindow();                  // Close window and OpenGL context

    return 0;
}
