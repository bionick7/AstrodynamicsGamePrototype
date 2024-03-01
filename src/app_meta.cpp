#include "app_meta.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "debug_console.hpp"
#include "global_state.hpp"
#include "assets.hpp"
#include <time.h>

const char* WINDOW_TITLE = "Astro navigation game prototype";

int prev_screen_width = 0;
int prev_screen_height = 0;

void InternalToggleFullScreen() {
    if (!GetSettingBool("allow_fullscreen", false))
        return;
    if (IsWindowFullscreen()) {
        SetWindowSize(
            GetMonitorWidth(prev_screen_width),
            GetMonitorHeight(prev_screen_height)
        );
    } else {
        prev_screen_width = GetScreenWidth();
        prev_screen_height = GetScreenHeight();
        int monitor = GetCurrentMonitor();
        //printf("Current monitor %d/%d (%s) %dx%d\n", monitor, GetMonitorCount(), GetMonitorName(monitor), GetMonitorWidth(monitor), GetMonitorHeight(monitor));
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
    //SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);

    InitWindow(1600, 900, WINDOW_TITLE);
    InitAudioDevice();
    Image img = LoadImage("resources/icons/app_icon.png");
    SetWindowIcon(img);
    
    SetExitKey(KEY_NULL);
    //SetTargetFPS(1e6);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    LogSetOutput("log.txt");
    LogToStdout(true);

    SetTraceLogLevel(LOG_INFO);
    SetTraceLogCallback(&CustomRaylibLog);
}

void AppMetaStep() {
    if (IsKeyPressed(KEY_F11)) {
        InternalToggleFullScreen();
    }
    // TODO: recompile raylib without screenshot stuff
    if (IsKeyPressed(KEY_F2)) {  // Disable when on steam
        //DatedScreenShot();
    }
}

void AppMetaClose() {
    CloseWindow();  // Close window and OpenGL context
    LogCloseOutputs();
}