#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"
#include "text_rendering.hpp"

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
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_PERIOD)) {
        time_scale *= 2;
    }
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_COMMA)) {
        time_scale /= 2;
    }
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_SPACE)) {
        paused = !paused;
    }
}

void Calendar::DrawUI() const {
    // timemath::Time scale (top-right corner)
    const int font_size = DEFAULT_FONT_SIZE;
    const char* text = TextFormat("II Time x %.1f", time_scale);
    if (!paused) text += 3;
    Vector2 pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text, font_size, 1).x - 10, 10 };
    text::DrawText(text, pos, Palette::ui_main);
    char text_date[100];
    GlobalGetNow().FormatAsDate(text_date, 100);
    pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text_date, font_size, 1).x - 10, 30 };
    text::DrawText(text_date, pos, Palette::ui_main);
}

bool Calendar::IsNewDay() const {
    return (int) prev_time.Days() != (int) time.Days();
}

timemath::Time Calendar::GetFrameElapsedGameTime() const {
    return time - prev_time;
}

void GameCamera::Make() {
    rl_camera.fovy = 90;
    rl_camera.projection = CAMERA_PERSPECTIVE;
    rl_camera.position = { 0.001f, 3.0f, 0.0f };
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
}

void GameCamera::Deserialize(const DataNode* data) {
    rl_camera.fovy = data->GetF("fovy");
    focus_object = RID(data->GetI("focus"));
    rl_camera.position.x = data->GetF("position_x");
    rl_camera.position.y = data->GetF("position_y");
    rl_camera.position.z = data->GetF("position_z");
}

void GameCamera::HandleInput() {

    float dist = Vector3Distance(rl_camera.position, rl_camera.target);
    Vector3 view_dir = Vector3Scale(Vector3Subtract(rl_camera.target, rl_camera.position), 1/dist);

    /*if (IsIdValidTyped(focus_object, EntityType::PLANET)) {
        rl_camera.target = (Vector3) (GetPlanet(focus_object)->position.cartesian / space_scale);
    }
    if (IsIdValidTyped(focus_object, EntityType::SHIP)) {
        rl_camera.target = (Vector3) (GetShip(focus_object)->position.cartesian / space_scale);
    }*/
    
    float scroll_ratio = 1 - 0.1 * GetMouseWheelMove();
    if (scroll_ratio != 1 && !GetUI()->IsPointBlocked(GetMousePosition(), 0)) {
        dist *= scroll_ratio;
    }

    bool translating = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && !translating) {
        double elevation = asin(view_dir.y);
        double azimuth = atan2(view_dir.z / cos(elevation), view_dir.x / cos(elevation));

        elevation -= GetMouseDelta().y * GetFrameTime();
        azimuth += GetMouseDelta().x * GetFrameTime();
        elevation = Clamp(elevation, -PI/2 + 1e-2, PI/2 - 1e-2);

        view_dir.x = cos(elevation) * cos(azimuth);
        view_dir.y = sin(elevation);
        view_dir.z = cos(elevation) * sin(azimuth);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && translating) {
        Ray prev_pos = GetMouseRay(Vector2Subtract(GetMousePosition(), GetMouseDelta()), rl_camera);
        Ray current_pos = GetMouseRay(GetMousePosition(), rl_camera);
        if (current_pos.position.y * current_pos.direction.y < 0) {
            if (fabs(prev_pos.direction.y) > 1e-3 && fabs(current_pos.direction.y) > 1e-3) {
                Vector3 prev_impact = Vector3Add(prev_pos.position, Vector3Scale(prev_pos.direction, prev_pos.position.y / prev_pos.direction.y));
                Vector3 current_impact = Vector3Add(current_pos.position, Vector3Scale(current_pos.direction, current_pos.position.y / current_pos.direction.y));
                Vector3 in_plane_delta = Vector3Subtract(current_impact, prev_impact);
                in_plane_delta = Vector3ClampValue(in_plane_delta, 0, 10 * GetFrameTime() * dist);
                rl_camera.target = Vector3Add(rl_camera.target, in_plane_delta);
            }
        }
    }
    
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_HOME)) {
        rl_camera.target = Vector3Zero();
        rl_camera.position = Vector3Subtract(rl_camera.target, Vector3Scale(view_dir, dist));
        //rl_camera.position = { 0.001f, 3.0f, 0.0f };
        focus_object = GetInvalidId();
    }

    rl_camera.position = Vector3Subtract(rl_camera.target, Vector3Scale(view_dir, dist));
}

bool GameCamera::IsInView(Vector3 render_pos) const {
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
