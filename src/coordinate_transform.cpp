#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"


void Calendar::Make(timemath::Time t0) {
    time_scale = 2048;
    paused = true;
    time = t0;
    prev_time = t0;
    current_migration_period = timemath::Time(86400 * 31 * 2);
    migration_arrrival_time = t0 + current_migration_period;
}

void Calendar::Serialize(DataNode* data) const {
    time.Serialize(data->SetChild("time", DataNode()));
    current_migration_period.Serialize(data->SetChild("current_migration_period", DataNode()));
    migration_arrrival_time.Serialize(data->SetChild("migration_arrrival_time", DataNode()));
    data->SetI("migration_arrrival_planet", (int) migration_arrrival_planet);
    data->SetF("time_scale", time_scale);
    data->Set("paused", paused ? "y": "n");
}

void Calendar::Deserialize(const DataNode* data) {
    time.Deserialize(data->GetChild("time"));
    current_migration_period.Deserialize(data->GetChild("current_migration_period"));
    migration_arrrival_time.Deserialize(data->GetChild("migration_arrrival_time"));
    migration_arrrival_planet = (entity_id_t) data->GetI("migration_arrrival_planet", (int) migration_arrrival_planet);
    time_scale = data->GetF("time_scale", time_scale);
    paused = strcmp(data->Get("paused", paused ? "y": "n"), "y") == 0;
}

timemath::Time Calendar::AdvanceTime(double delta_t) {
    prev_time = time;
    if (paused) return time;
    
    time = time + delta_t * time_scale;
    if (migration_arrrival_time < GlobalGetNow()){
        migration_arrrival_time = migration_arrrival_time + current_migration_period;
        USER_INFO("New migrants arrive")
    }

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
    DrawTextEx(GetCustomDefaultFont(), text, pos, FONT_SIZE, 1, MAIN_UI_COLOR);
    char text_date[100];
    GlobalGetNow().FormatAsDate(text_date, 100);
    pos = { GetScreenWidth() - MeasureTextEx(GetCustomDefaultFont(), text_date, FONT_SIZE, 1).x - 10, 30 };
    DrawTextEx(GetCustomDefaultFont(), text_date, pos, FONT_SIZE, 1, MAIN_UI_COLOR);

    // Migration progress
    double t_val = 1.0 - (migration_arrrival_time - GlobalGetNow()).Seconds() / current_migration_period.Seconds();
    int progress_x = t_val * GetScreenWidth();
    DrawRectangle(0, 1, progress_x, 2, MAIN_UI_COLOR);
    const int collider_rec_width = 16;
    Rectangle mouse_collider = { progress_x - collider_rec_width/2, 0, collider_rec_width, collider_rec_width};
    if (CheckCollisionPointRec(GetMousePosition(), mouse_collider) || GetMousePosition().y < 4) {
        char buffer[30];
        char* buffer2 = (migration_arrrival_time - GlobalGetNow()).FormatAsTime(buffer, 30);
        strncpy(buffer2, GetPlanet(migration_arrrival_planet)->name, 30 - (buffer2 - buffer));
        Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), buffer, 16, 1);
        if (progress_x > GetScreenWidth() - text_size.x - 200) {
            progress_x = GetScreenWidth() - text_size.x - 200;
        }
        DrawTextEx(GetCustomDefaultFont(), buffer, (Vector2) { progress_x, 1 }, 16, 1, MAIN_UI_COLOR);
    }
}

bool Calendar::IsNewDay() const {
    return (int) prev_time.Days() != (int) time.Days();
}

timemath::Time Calendar::GetFrameElapsedGameTime() const {
    return time - prev_time;
}

Calendar* GetCalendar() {
    return &GlobalGetState()->calendar;
}

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

}

Vector2 GetMousePositionInWorld() {
    return GlobalGetState()->c_transf.InvTransformV(GetMousePosition());
}

CoordinateTransform* GetScreenTransform() {
    return &GlobalGetState()->c_transf;
}