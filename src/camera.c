#include "camera.h"
#include "global_state.h"
#include "ui.h"
#include "debug_drawing.h"

void CameraMake(DrawCamera* cam){
    cam->space_scale = 1e-6;
    cam->time_scale = 2048;
    cam->paused = true;
    cam->focus = (Vector2) {0};
}

Vector2 _FlipY(Vector2 a) {
    return (Vector2) {a.x, -a.y};
}

Vector2 CameraTransformV(const DrawCamera* cam, Vector2 p) {
    return Vector2Add(
        Vector2Scale(
        _FlipY(Vector2Subtract(p, cam->focus)),
        cam->space_scale),
        GetScreenCenter()
    );
}

Vector2 _CameraInvTransformV(double scale, Vector2 focus, Vector2 p) {
    return Vector2Add(_FlipY(
        Vector2Scale(
        Vector2Subtract(p, GetScreenCenter()),
        1 / scale)),
        focus
    );
}

Vector2 CameraInvTransformV(const DrawCamera* cam, Vector2 p) {
    return _CameraInvTransformV(cam->space_scale, cam->focus, p);
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
    float scroll_ratio = 1 - atan(-0.1 * GetMouseWheelMove());

    if (scroll_ratio != 1) {
        // ((P - c) / s1)*v + f1 = ((P - c) / s2)*v + f2
        // ((P - c) / s1)*v - ((P - c) / s2) + f1 = f2
        cam->focus = Vector2Subtract(
            _CameraInvTransformV(cam->space_scale, cam->focus, GetMousePosition()),
            _CameraInvTransformV(cam->space_scale * scroll_ratio, (Vector2){0, 0}, GetMousePosition())
        );
        cam->space_scale *= scroll_ratio;
    }
    //printf("Scroll %f\n", cam->space_scale);

    if (IsKeyDown(KEY_W)) {
        cam->focus.y += delta_t / cam->space_scale * 300;
    }
    if (IsKeyDown(KEY_S)) {
        cam->focus.y -= delta_t / cam->space_scale * 300;
    }
    if (IsKeyDown(KEY_A)) {
        cam->focus.x -= delta_t / cam->space_scale * 300;
    }
    if (IsKeyDown(KEY_D)) {
        cam->focus.x += delta_t / cam->space_scale * 300;
    }

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
    Vector2 pos = (Vector2) { GetScreenWidth() - MeasureText(text, 20) - 10, 10 };
    DrawTextEx(GetCustomDefaultFont(), text, pos, 20, 1, WHITE);
}

Vector2 GetMousePositionInWorld() {
    return CameraInvTransformV(&GlobalGetState()->camera, GetMousePosition());
}

DrawCamera* GetMainCamera() {
    return &GlobalGetState()->camera;
}