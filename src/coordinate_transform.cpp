#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"

void CoordinateTransform::Make(){
    space_scale = 1e-6;
    time_scale = 2048;
    paused = true;
    focus = {0};
}

Vector2 _FlipY(Vector2 a) {
    return {a.x, -a.y};
}

Vector2 CoordinateTransform::TransformV(Vector2 p) const {
    return Vector2Add(
        Vector2Scale(
        _FlipY(Vector2Subtract(p, focus)),
        space_scale),
        GetScreenCenter()
    );
}

Vector2 _InvTransformV(double scale, Vector2 focus, Vector2 p) {
    return Vector2Add(_FlipY(
        Vector2Scale(
        Vector2Subtract(p, GetScreenCenter()),
        1 / scale)),
        focus
    );
}

Vector2 CoordinateTransform::InvTransformV(Vector2 p) const {
    return _InvTransformV(space_scale, focus, p);
}

double CoordinateTransform::TransformS(double p) const {
    return p * space_scale;
}

double CoordinateTransform::InvTransformS(double p) const {
    return p / space_scale;
}

void CoordinateTransform::TransformBuffer(Vector2* buffer, int buffer_size) const {
    for(int i=0; i < buffer_size; i++) {
        buffer[i] = TransformV(buffer[i]);
    }
}

time_type CoordinateTransform::AdvanceTime(time_type t0, double delta_t) const {
    if (paused) return t0;
    return t0 + delta_t * time_scale;
}

void CoordinateTransform::HandleInput(double delta_t) {
    float scroll_ratio = 1 - atan(-0.1 * GetMouseWheelMove());

    if (scroll_ratio != 1) {
        // ((P - c) / s1)*v + f1 = ((P - c) / s2)*v + f2
        // ((P - c) / s1)*v - ((P - c) / s2) + f1 = f2
        focus = Vector2Subtract(
            _InvTransformV(space_scale, focus, GetMousePosition()),
            _InvTransformV(space_scale * scroll_ratio, {0, 0}, GetMousePosition())
        );
        space_scale *= scroll_ratio;
    }
    //printf("Scroll %f\n", space_scale);

    /*if (IsKeyDown(KEY_W)) {
        focus.y += delta_t / space_scale * 300;
    }
    if (IsKeyDown(KEY_S)) {
        focus.y -= delta_t / space_scale * 300;
    }
    if (IsKeyDown(KEY_A)) {
        focus.x -= delta_t / space_scale * 300;
    }
    if (IsKeyDown(KEY_D)) {
        focus.x += delta_t / space_scale * 300;
    }*/
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        focus.x -= GetMouseDelta().x / space_scale;
        focus.y += GetMouseDelta().y / space_scale;
    }

    if (IsKeyPressed(KEY_PERIOD)) {
        time_scale *= 2;
    }
    if (IsKeyPressed(KEY_COMMA)) {
        time_scale /= 2;
    }
    if (IsKeyPressed(KEY_SPACE)) {
        paused = !paused;
    }
}

void CoordinateTransform::DrawUI() const {
    const char* text = TextFormat("II Time x %.1f", time_scale);
    if (!paused) text += 3;
    Vector2 pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text, 20, 1).x - 10, 10 };
    DrawTextEx(GetCustomDefaultFont(), text, pos, 20, 1, MAIN_UI_COLOR);
    char text_date[100];
    FormatDate(text_date, 100, GlobalGetNow());
    pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text_date, 20, 1).x - 10, 30 };
    DrawTextEx(GetCustomDefaultFont(), text_date, pos, 20, 1, MAIN_UI_COLOR);
}

Vector2 GetMousePositionInWorld() {
    return GlobalGetState()->c_transf.InvTransformV(GetMousePosition());
}

CoordinateTransform* GetScreenTransform() {
    return &GlobalGetState()->c_transf;
}