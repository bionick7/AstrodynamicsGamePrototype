#include "camera.h"
#include "global_state.h"
#include "ui.h"

void CameraMake(DrawCamera* cam){
    cam->space_scale = 1e-6;
    cam->time_scale = 2048;
    cam->paused = true;
}

Vector2 CameraTransformV(const DrawCamera* cam, Vector2 p) {
    p.y = -p.y;
    return Vector2Add(
        Vector2Scale(p, cam->space_scale),
        SCREEN_CENTER
    );
}

Vector2 CameraInvTransformV(const DrawCamera* cam, Vector2 p) {
    p.y = SCREEN_HEIGHT - p.y;
    return Vector2Scale(
        Vector2Subtract(p, SCREEN_CENTER),
        1 / cam->space_scale
    );
}

double CameraTransformS(const DrawCamera* cam, double p) {
    return p * cam->space_scale;
}

double CameraInvTransformS(const DrawCamera* cam, double p) {
    return p / cam->space_scale;
}

void CameraTransformBuffer(const DrawCamera* cam, Vector2* buffer, int buffer_size) {
    for(int i=0; i < buffer_size; i++) {
        buffer[i] = CameraTransformV(cam, buffer[i]);
    }
}

time_type CameraAdvanceTime(const DrawCamera* cam, time_type t0, double delta_t) {
    if (cam->paused) return t0;
    return t0 + delta_t * cam->time_scale;
}

void CameraHandleInput(DrawCamera* cam, double delta_t) {
    cam->space_scale *= 1 - atan(-0.1 * GetMouseWheelMove());
    //printf("Scroll %f\n", cam->space_scale);

    if (IsKeyPressed(KEY_PERIOD)) {
        cam->time_scale *= 2;
    }
    if (IsKeyPressed(KEY_COMMA)) {
        cam->time_scale /= 2;
    }
    if (IsKeyPressed(KEY_SPACE)) {
        cam->paused = !cam->paused;
    }
}

void CameraDrawUI(const DrawCamera* cam) {
    const char* text = TextFormat("II Time x %.1f", cam->time_scale);
    if (!cam->paused) text += 3;
    Vector2 pos = (Vector2) { SCREEN_WIDTH - MeasureText(text, 20) - 10, 10 };
    DrawTextEx(GetCustomDefaultFont(), text, pos, 20, 1, WHITE);
}

Vector2 GetMousePositionInWorld() {
    return CameraInvTransformV(&GlobalGetState()->camera, GetMousePosition());
}

DrawCamera* GetMainCamera() {
    return &GlobalGetState()->camera;
}