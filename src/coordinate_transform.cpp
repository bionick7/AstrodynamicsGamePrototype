#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"


void Calendar::Make(timemath::Time t0) {
    time_scale = 2048*8;
    paused = true;
    time = t0;
    prev_time = t0;
}

void Calendar::Serialize(DataNode* data) const {
    time.Serialize(data, "time");
    data->SetF("time_scale", time_scale);
    data->Set("paused", paused ? "y": "n");
}

void Calendar::Deserialize(const DataNode* data) {
    time.Deserialize(data, "time");
    time_scale = data->GetF("time_scale", time_scale);
    paused = strcmp(data->Get("paused", paused ? "y": "n"), "y") == 0;
}

timemath::Time Calendar::AdvanceTime(double delta_t) {
    prev_time = time;
    if (paused) return time;
    
    time = time + delta_t * time_scale;
    return time;
}

void Calendar::HandleInput(double delta_t) {
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

void Calendar::DrawUI() const {
    // timemath::Time scale (top-right corner)
    const int FONT_SIZE = 16;
    const char* text = TextFormat("II Time x %.1f", time_scale);
    if (!paused) text += 3;
    Vector2 pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text, FONT_SIZE, 1).x - 10, 10 };
    DrawTextEx(GetCustomDefaultFont(), text, pos, FONT_SIZE, 1, Palette::ui_main);
    char text_date[100];
    GlobalGetNow().FormatAsDate(text_date, 100);
    pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text_date, FONT_SIZE, 1).x - 10, 30 };
    DrawTextEx(GetCustomDefaultFont(), text_date, pos, FONT_SIZE, 1, Palette::ui_main);
}

bool Calendar::IsNewDay() const {
    return (int) prev_time.Days() != (int) time.Days();
}

timemath::Time Calendar::GetFrameElapsedGameTime() const {
    return time - prev_time;
}
/*
void CoordinateTransform::Make(){
    space_scale = 1e-6;
    focus = {0};
}

void CoordinateTransform::Serialize(DataNode* data) const {
    data->SetF("log_space_scale", log10(space_scale));
    data->SetF("focus_x", focus.x);
    data->SetF("focus_y", focus.y);
}

void CoordinateTransform::Deserialize(const DataNode* data) {
    space_scale = pow(10, data->GetF("log_space_scale", log10(space_scale)));
    focus.x = data->GetF("focus_x", focus.x);
    focus.y = data->GetF("focus_y", focus.y);
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

void CoordinateTransform::HandleInput(double delta_t) {
    float scroll_ratio = 1 + 0.1 * GetMouseWheelMove();

    auto current_focus = GetGlobalState()->current_focus;

    if (scroll_ratio != 1 && !GetUI()->scroll_lock) {
        // ((P - c) / s1)*v + f1 = ((P - c) / s2)*v + f2
        // ((P - c) / s1)*v - ((P - c) / s2) + f1 = f2
        focus = Vector2Subtract(
            _InvTransformV(space_scale, focus, GetMousePosition()),
            _InvTransformV(space_scale * scroll_ratio, {0, 0}, GetMousePosition())
        );
        space_scale *= scroll_ratio;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        focus.x -= GetMouseDelta().x / space_scale;
        focus.y += GetMouseDelta().y / space_scale;
    }

}

Vector2 GetMousePositionInWorld() {
    return GetCoordinateTransform()->InvTransformV(GetMousePosition());
}*/

void GameCamera::Make() {
    rl_camera.fovy = 90;
    rl_camera.projection = CAMERA_PERSPECTIVE;
    rl_camera.position = { 1.0f, 1.0f, 0.0f };
    rl_camera.target = Vector3Zero();
    rl_camera.up = { 0.0f, 1.0f, 0.0f };
    focus_object = GetInvalidId();
}

void GameCamera::Serialize(DataNode* data) const {
    data->SetF("fovy", rl_camera.fovy);
    data->SetI("focus", focus_object.AsInt());
    data->SetF("position_x", rl_camera.position.x);
    data->SetF("position_y", rl_camera.position.y);
    data->SetF("position_z", rl_camera.position.z);
    //data->SetF("target_x", rl_camera.target.x);
    //data->SetF("target_y", rl_camera.target.y);
    //data->SetF("target_z", rl_camera.target.z);
    //data->SetF("up_x", rl_camera.up.x);
    //data->SetF("up_y", rl_camera.up.y);
    //data->SetF("up_z", rl_camera.up.z);
}

void GameCamera::Deserialize(const DataNode* data) {
    rl_camera.fovy = data->GetF("fovy");
    focus_object = RID(data->GetI("focus"));
    rl_camera.position.x = data->GetF("position_x");
    rl_camera.position.y = data->GetF("position_y");
    rl_camera.position.z = data->GetF("position_z");
    //rl_camera.target.x = data->GetF("target_x");
    //rl_camera.target.y = data->GetF("target_y");
    //rl_camera.target.z = data->GetF("target_z");
    //rl_camera.up.x = data->GetF("up_x");
    //rl_camera.up.y = data->GetF("up_y");
    //rl_camera.up.z = data->GetF("up_z");
}

void GameCamera::HandleInput() {
    
    if (IsKeyPressed(KEY_HOME)) {
        rl_camera.target = Vector3Zero();
        rl_camera.position = { 1.0f, 1.0f, 0.0f };
        focus_object = GetInvalidId();
    }

    float dist = Vector3Distance(rl_camera.position, rl_camera.target);
    Vector3 view_dir = Vector3Scale(Vector3Subtract(rl_camera.target, rl_camera.position), 1/dist);

    if (IsIdValidTyped(focus_object, EntityType::PLANET)) {
        rl_camera.target = (Vector3) (GetPlanet(focus_object)->position.cartesian / space_scale);
    }
    if (IsIdValidTyped(focus_object, EntityType::SHIP)) {
        rl_camera.target = (Vector3) (GetShip(focus_object)->position.cartesian / space_scale);
    }
    
    float scroll_ratio = 1 - 0.1 * GetMouseWheelMove();
    if (scroll_ratio != 1 && !GetUI()->scroll_lock) {
        dist *= scroll_ratio;
    }

    // TODO: manipulate view_dir
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        double elevation = asin(view_dir.y);
        double azimuth = atan2(view_dir.z / cos(elevation), view_dir.x / cos(elevation));

        elevation -= GetMouseDelta().y * GetFrameTime();
        azimuth += GetMouseDelta().x * GetFrameTime();
        elevation = Clamp(elevation, -PI/2 + 1e-2, PI/2 - 1e-2);

        view_dir.x = cos(elevation) * cos(azimuth);
        view_dir.y = sin(elevation);
        view_dir.z = cos(elevation) * sin(azimuth);
    }

    rl_camera.position = Vector3Subtract(rl_camera.target, Vector3Scale(view_dir, dist));
}

bool GameCamera::IsInView(Vector3 render_pos) const {
    Vector3 camera_dir = Vector3Subtract(rl_camera.target, rl_camera.position);
    return Vector3DotProduct(
        Vector3Subtract(render_pos, rl_camera.position),
        Vector3Subtract(rl_camera.target, rl_camera.position)
    ) > 0;
}

bool GameCamera::IsInView(DVector3 render_pos) const {
    return IsInView(WorldToRender(render_pos));
}

Vector2 GameCamera::GetScreenPos(DVector3 world_pos) const {
    if (!IsInView(world_pos)) {
        return { -1000.0f, -1000.0f };
    }
    return GetWorldToScreen(WorldToRender(world_pos), rl_camera);
}

float GameCamera::MeasurePixelSize(Vector3 render_pos) const {
    return Vector3Distance(rl_camera.position, render_pos) * tan(rl_camera.fovy/2.0) / GetScreenHeight();
}

Matrix GameCamera::ViewMatrix() const {
    return GetCameraMatrix(rl_camera);
}

Matrix GameCamera::ProjectionMatrix() const {
    double ar = GetScreenWidth() / (double) GetScreenHeight();
    return MatrixPerspective(rl_camera.fovy*DEG2RAD, ar, 0.01, 1000.0);
}
