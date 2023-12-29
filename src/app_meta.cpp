#include "app_meta.hpp"
#include "constants.hpp"
#include <time.h>

const char* WINDOW_TITLE = "Astro navigation game prototype";

int prev_screen_width = SCREEN_WIDTH;
int prev_screen_height = SCREEN_HEIGHT;

void InternalToggleFullScreen() {
    if (IsWindowFullscreen()) {
        SetWindowSize(
            GetMonitorWidth(prev_screen_width),
            GetMonitorHeight(prev_screen_height)
        );
    } else {
        prev_screen_width = GetScreenWidth();
        prev_screen_height = GetScreenHeight();
        int monitor = GetCurrentMonitor();
        printf("Current monitor %d/%d (%s) %dx%d\n", monitor, GetMonitorCount(), GetMonitorName(monitor), GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        SetWindowSize(
            GetMonitorWidth(monitor),
            GetMonitorHeight(monitor)
        );
    }
    ToggleFullscreen();  // Doesn't work on (my) linux
}

void DatedScreenShot() {
    time_t timer = time(NULL);
    tm* tm_info = localtime(&timer);
    char buffer[42] = "screenshots/";
    strftime(&buffer[12], 30, "%Y_%m_%d %H_%M_%S.png", tm_info);
    TakeScreenshot(buffer);  // Doesn't work in other directories
}

void AppMetaInit() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    InitAudioDevice();
}

void AppMetaStep() {
    if (IsKeyPressed(KEY_F11)) {
        InternalToggleFullScreen();
    }
    // TODO: recompile raylib without screenshot stuff
    if (IsKeyPressed(KEY_F2)) {  // Disable when on steam
        DatedScreenShot();
    }
}