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
        //INFO("Current monitor %d/%d (%s) %dx%d\n", monitor, GetMonitorCount(), GetMonitorName(monitor), GetMonitorWidth(monitor), GetMonitorHeight(monitor));
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
    // Setup logging system first
    SetLoadFileDataCallback(assets::GetResourceBytes);
    SetLoadFileTextCallback(assets::GetResourceText);
    SetTraceLogLevel(LOG_INFO);
    SetTraceLogCallback(CustomRaylibLog);

    //LogSetOutput("log.txt");
    LogToStdout(true);

    int fps_cap = GetSettingNum("fps_cap");  // negative cap means unlimited
    bool v_sync = GetSettingBool("v_sync");

    // ConfigFlags are called before window creation
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    if (v_sync && fps_cap >= 0) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(1600, 900, WINDOW_TITLE);
    InitAudioDevice();

    SetWindowIcon(assets::GetImage("resources/icons/app_icon.png"));
    
    SetExitKey(KEY_NULL);
    if (fps_cap >= 0) {
        SetTargetFPS(fps_cap);
    } else {
        SetTargetFPS(1e9);
    }
}

void AppMetaStep() {
    if (IsKeyPressed(KEY_F11)) {
        InternalToggleFullScreen();
    }
    if (IsKeyPressed(KEY_F2)) {  // Disable when on steam
        //DatedScreenShot();
    }
}

void AppMetaClose() {
    CloseWindow();  // Close window and OpenGL context
}