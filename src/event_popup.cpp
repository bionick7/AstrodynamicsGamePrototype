#include "event_popup.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "render_server.hpp"
#include "global_state.hpp"
    
int total_height = 0;
const int header_height = 25;
int face_height = 0;
const int footer_height = 40;

int draw_state = 0;

struct { int width = 0, height = 0; } render_texture_size;
RenderTexture2D rendertexture;

void event_popup::MakeEventPopup(int x, int y, int width, int height) {
    total_height = height + header_height + footer_height;
    ui::CreateNew(x, y, width, total_height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::POPUPS);
    ui::Enclose();
    ui::PushInset(header_height);

    ui::PushHSplit(width - 30, width);
    ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 0);
    button_state_flags::T button_state = ui::AsButton();
    HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
    if (button_state & button_state_flags::JUST_PRESSED) {
        
    }
    ui::WriteEx("X", text_alignment::CENTER, false);
    ui::Pop();  // HSplit

    ui::Pop();  // Inset
    draw_state = 1;
    face_height = 0;
}

void event_popup::MakeEventPopupCentered(int width, int height) {
    MakeEventPopup((GetScreenWidth() - width) / 2, (GetScreenHeight() - height) / 2,
                   width, height);
}

void event_popup::EndEventPopup() {
    if (draw_state != 4) {
        FAIL("Invalid operation for this draw state")
    }
    ui::Pop();  // Inset
    ui::Pop();  // Final
    draw_state = 0;
}

void event_popup::BeginTurntableFace(int height, float angular_vel, float pitch) {
    if (draw_state != 1) {
        FAIL("Cannot start drawing face in this draw state")
    }
    if (height <= 0) return;

    face_height = height;
    ui::PushInset(face_height);
    draw_state = 2;

    // Define turntable camera
    Camera3D camera;
    float yaw = GetTime() * angular_vel;
    const float distance = 5;
    camera.fovy = 45;
    camera.position = {
        cosf(yaw) * cosf(pitch) * distance,
        -sinf(pitch) * distance,
        sinf(yaw) * cosf(pitch) * distance,
    };
    camera.target = Vector3Zero();
    camera.up = { 0, 1, 0 };
    camera.projection = CAMERA_PERSPECTIVE;

    // Set up render environment
    int target_render_width = ui::Current()->width;
    int target_render_height = ui::Current()->height;
    if (render_texture_size.width != target_render_width || 
        render_texture_size.height != target_render_height
    ) {
        render_texture_size.width = target_render_width;
        render_texture_size.height = target_render_height;
        if (IsRenderTextureReady(rendertexture))
            UnloadRenderTextureWithDepth(rendertexture);
        rendertexture = LoadRenderTextureWithDepth(target_render_width, target_render_height);
    }

    EndTextureMode();
    BeginTextureMode(rendertexture);
    BeginMode3D(camera);
    ClearBackground(ColorAlpha(Palette::bg, 0));
    //ClearBackground(PINK);
}

void event_popup::BeginBody() {
    if (draw_state == 1) {
        // Skip header
    } else if (draw_state == 2) {
        EndMode3D();
        EndTextureMode();
        BeginTextureMode(GetRenderServer()->render_targets[1]);
        Rectangle source_rect = { 0, 0, rendertexture.texture.width, rendertexture.texture.height };
        ui::Current()->DrawTexture(rendertexture.texture, source_rect,
                                   face_height, WHITE, false);
        ui::Pop();  // Header inset
    } else {
        FAIL("Cannot start drawing face in this draw state")
    }
    int body_height = total_height - header_height - face_height - footer_height;
    ui::PushInset(body_height);
    draw_state = 3;
}

void event_popup::EndBody() {
    if (draw_state != 3) {
        FAIL("Invalid operation for this draw state")
    }
    ui::Pop();  // Inset
    ui::PushInset(footer_height);
    draw_state = 4;
}

bool event_popup::Choice(const char *desc, int number, int total) {
    if (draw_state != 4) {
        FAIL("Invalid operation for this draw state")
    }
    int w = ui::Current()->width;
    ui::PushHSplit((number * w) / total, ((number+1) * w) / total);

    button_state_flags::T button_state = ui::AsButton();
    HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
    if (button_state & button_state_flags::HOVER) {
        ui::Current()->text_color = Palette::interactable_main;
        ui::WriteEx(desc, text_alignment::CENTER, false);
        ui::Current()->text_color = Palette::ui_main;
    } else {
        ui::WriteEx(desc, text_alignment::CENTER, false);
    }

    ui::Pop();  // HSplit
    return button_state & button_state_flags::JUST_PRESSED;
}
