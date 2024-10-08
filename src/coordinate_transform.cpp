#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"
#include "text_rendering.hpp"
#include "debug_console.hpp"

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

void Calendar::FastForward(timemath::Time target_delta) {
    timelapse_origin = time;
    timelapse_target = time + target_delta;
    timelapse = true;
}

void Calendar::Update(double delta_t) {
    if (!GetGlobalState()->IsKeyBoardFocused()) {
        if (IsKeyPressed(KEY_PERIOD)) {
            time_scale *= 2;
        }
        if (IsKeyPressed(KEY_COMMA)) {
            time_scale /= 2;
        }
        if (IsKeyDown(KEY_SPACE) && !timelapse) {
            //paused = !paused;
            FastForward(timemath::Time::Day());
        }
    }
    
    prev_time = time;
    if (timelapse) {
        time = time + (timelapse_target - timelapse_origin) * delta_t;
        if (time >= timelapse_target) {
            time = timelapse_target;
            timelapse = false;
        }
    } else if (!paused) {
        time = time + delta_t * time_scale;
    }

    global_vars::Set("time_days", time.Days());
}

void Calendar::DrawUI() const {
    const int font_size = DEFAULT_FONT_SIZE;

    Font font = GetCustomDefaultFont(font_size);

    //const char* text = TextFormat("%s Time x %.1f", paused ? ICON_PAUSE : ICON_PLAY, time_scale);
    //Vector2 pos = { GetScreenWidth() - MeasureTextEx(font, text, font_size, 1).x - 10, 10 };
    //text::DrawTextEx(font, text, pos, font_size, 1, Palette::ui_main, Palette::bg, GetScreenRect(), 0);
    
    StringBuilder sb;
    sb.AddDate(GlobalGetNow());
    Vector2 pos = { GetScreenWidth() - MeasureTextEx(font, sb.c_str, font_size, 1).x - 10, 30 };
    text::DrawTextEx(font, sb.c_str, pos, font_size, 1, Palette::ui_main, Palette::bg, GetScreenRect(), 0);
}

bool Calendar::IsNewDay() const {
    return (int) prev_time.Days() != (int) time.Days();
}

timemath::Time Calendar::GetFrameElapsedGameTime() const {
    return time - prev_time;
}

double GameCamera::MacroNearClipPlane() const {
    return fmaxf(macro_view_distance, 0.01) * 0.01;
}

double GameCamera::MacroFarClipPlane() const {
    return fmaxf(macro_view_distance, 0.01) * 1000;
}

double GameCamera::MicroNearClipPlane() const {
    return 0.01;
}

double GameCamera::MicroFarClipPlane() const {
    return 1000;
}

void GameCamera::Make() {
    macro_camera.fovy = GetSettingNum("fov_deg", 90);
    macro_camera.projection = CAMERA_PERSPECTIVE;
    macro_camera.position = { 0.1f, 3.0f, 0.0f };
    macro_camera.target = Vector3Zero();
    macro_camera.up = { 0.0f, 1.0f, 0.0f };

    micro_camera.fovy = GetSettingNum("fov_deg", 90);
    micro_camera.projection = CAMERA_PERSPECTIVE;
    micro_camera.position = Vector3Scale(macro_camera.position, scale_ratio);
    micro_camera.target = Vector3Zero();
    micro_camera.up = { 0.0f, 1.0f, 0.0f };

    world_focus = DVector3::Zero();
    view_direction = Vector3Scale(Vector3Normalize(macro_camera.position), -1);
    macro_view_distance = 3;
    focus_object = GetInvalidId();
}

void GameCamera::Serialize(DataNode* data) const {
    data->SetF("focus_point_x", world_focus.x);
    data->SetF("focus_point_y", world_focus.y);
    data->SetF("focus_point_z", world_focus.z);
    data->SetF("view_direction_x", view_direction.x);
    data->SetF("view_direction_y", view_direction.y);
    data->SetF("view_direction_z", view_direction.z);
    data->SetF("view_distance", macro_view_distance);
    data->SetI("focus", focus_object.AsInt());
}

void GameCamera::Deserialize(const DataNode* data) {
    world_focus.x = data->GetF("focus_point_x");
    world_focus.y = data->GetF("focus_point_y");
    world_focus.z = data->GetF("focus_point_z");
    view_direction.x = data->GetF("view_direction_x");
    view_direction.y = data->GetF("view_direction_y");
    view_direction.z = data->GetF("view_direction_z");
    macro_view_distance = data->GetF("view_distance");
    focus_object = RID(data->GetI("focus"));
}

void GameCamera::HandleInput() {
    if (IsIdValidTyped(focus_object, EntityType::PLANET)) {
        world_focus = GetPlanet(focus_object)->position.cartesian;
    }
    if (IsIdValidTyped(focus_object, EntityType::SHIP)) {
        const Ship* ship = GetShip(focus_object);
        if (ship->IsParked()) {
            const Planet* planet = GetPlanet(ship->GetParentPlanet());
            Orbit ship_orbit;
            planet->GetRandomOrbit(0, &ship_orbit);
            world_focus = ship_orbit.GetPosition(GlobalGetNow()).cartesian + planet->position.cartesian;
        } else {
            world_focus = ship->position.cartesian;
        }
    }
    
    float scroll_ratio = 1 - 0.1 * GetMouseWheelMove();
    if (scroll_ratio != 1 && !GetUI()->IsPointBlocked(GetMousePosition(), 0)) {
        macro_view_distance *= scroll_ratio;
    }

    bool translating = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && !translating) {
        // Camera Rotation
        double elevation = asin(view_direction.y);
        double azimuth = atan2(view_direction.z / cos(elevation), view_direction.x / cos(elevation));

        double camera_sensitivity = GetSettingNum("camera_rotation_sensitivity");
        elevation -= GetMouseDelta().y * camera_sensitivity;
        azimuth += GetMouseDelta().x * camera_sensitivity;
        elevation = Clamp(elevation, -PI/2 + 1e-2, PI/2 - 1e-2);

        view_direction.x = cos(elevation) * cos(azimuth);
        view_direction.y = sin(elevation);
        view_direction.z = cos(elevation) * sin(azimuth);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && translating) {
        // Camera Translation
        Ray prev_pos = GetMouseRay(Vector2Subtract(GetMousePosition(), GetMouseDelta()), macro_camera);
        Ray current_pos = GetMouseRay(GetMousePosition(), macro_camera);
        if (current_pos.position.y * current_pos.direction.y < 0) {                          // Mouse 'grabs' the origin plane ...
            if (fabs(prev_pos.direction.y) > 1e-3 && fabs(current_pos.direction.y) > 1e-3) { // ... but not at the horizon
                Vector3 prev_impact = Vector3Add(prev_pos.position, Vector3Scale(prev_pos.direction, prev_pos.position.y / prev_pos.direction.y));
                Vector3 current_impact = Vector3Add(current_pos.position, Vector3Scale(current_pos.direction, current_pos.position.y / current_pos.direction.y));
                Vector3 in_plane_delta = Vector3Subtract(current_impact, prev_impact);
                in_plane_delta = Vector3ClampValue(in_plane_delta, 0, 10 * GetFrameTime() * macro_view_distance);
                world_focus = world_focus + DVector3(in_plane_delta) * macro_scale;
            }
        }
        focus_object = GetInvalidId();
    }

    if (IsKeyPressed(KEY_F)) {
        // Enable camera track ( F ocus) on currently selected object
        if (IsIdValid(GetGlobalState()->focused_ship)) {
            focus_object = GetGlobalState()->focused_ship;
        } else if (IsIdValid(GetGlobalState()->focused_planet)) {
            focus_object = GetGlobalState()->focused_planet;
        }
    }
    
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_HOME)) {
        // Reset Camera
        world_focus = DVector3::Zero();
        focus_object = GetInvalidId();
    }
    macro_camera.target = Vector3Zero();

    macro_view_distance = Clamp(macro_view_distance, 1e-6, 100);  // Zoom limits

    macro_camera.target = WorldToMacro(world_focus);
    macro_camera.position = Vector3Subtract(macro_camera.target, Vector3Scale(view_direction, macro_view_distance));
    if (macro_view_distance < 1e-3) {  // Avoid jitter due to very small distances
        macro_camera.target = Vector3Add(macro_camera.position, Vector3Scale(view_direction, 1e-3));
    }

    micro_camera.target = Vector3Zero();  // Assumes object in view is always at (0,0,0)
    micro_camera.position = Vector3Scale(view_direction, -macro_view_distance*scale_ratio);
}

bool GameCamera::IsInView(Vector3 macro_pos) const {
    return Vector3DotProduct(
        Vector3Subtract(macro_pos, macro_camera.position),
        Vector3Subtract(macro_camera.target, macro_camera.position)
    ) > 0;
}

bool GameCamera::IsInView(DVector3 world_pos) const {
    return IsInView(WorldToMacro(world_pos));
}

Vector2 GameCamera::GetScreenPos(DVector3 world_pos) const {
    if (!IsInView(world_pos)) {
        return { -1000.0f, -1000.0f };
    }
    return GetWorldToScreen(WorldToMacro(world_pos), macro_camera);
}

float GameCamera::MeasurePixelSize(Vector3 render_pos) const {
    return Vector3Distance(macro_camera.position, render_pos) * tan(macro_camera.fovy/2.0*DEG2RAD) / GetScreenHeight() * 2.0;
}

Matrix GameCamera::ViewMatrix() const {
    return GetCameraMatrix(macro_camera);
}

Matrix GameCamera::ProjectionMatrix() const {
    double ar = GetScreenWidth() / (double) GetScreenHeight();
    return MatrixPerspective(macro_camera.fovy*DEG2RAD, ar, MacroNearClipPlane(), MacroFarClipPlane());
}
