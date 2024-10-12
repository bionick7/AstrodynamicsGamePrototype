#include "event_popup.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "render_server.hpp"
#include "global_state.hpp"
#include "list.hpp"

List<Popup> popup_list = List<Popup>();

int total_height = 0;
const int header_height = 25;
int face_height = 0;
const int footer_height = 40;
int draw_state = 0;

void Popup::Draw(event_popup::PopupEvents* events, int relative_offset) const {
    // Popup test
    event_popup::MakeEventPopup((int) x, (int) y, width, height, title, events, relative_offset);
    event_popup::EmbeddedSceneFace(embedded_scene, face_height);

    event_popup::BeginBody();

    ui::Write(description);

    event_popup::EndBody();
    if (event_popup::Choice("Gotcha Chief!", 0, 1)) {
        events->request_close = true;
    }
    event_popup::EndEventPopup();
}

Popup* event_popup::AddPopup(int width, int height, int face_height) {
    int index = popup_list.AllocForAppend();  // Avoid copying heavy object
    popup_list[index].width = width;
    popup_list[index].height = height;
    popup_list[index].face_height = face_height;
    popup_list[index].embedded_scene = GetInvalidId();
    popup_list[index].x = (GetScreenWidth() - width) / 2.0f;
    popup_list[index].y = (GetScreenHeight() - height) / 2.0f;
    popup_list[index].dragging = false;
    return &popup_list[index];
}

void event_popup::UpdateAllPopups() {
    bool mouse_blocked = false;
    for(int i=popup_list.Count()-1; i >= 0; i--) {
        RID scene_id = popup_list[i].embedded_scene;
        if (IsIdValidTyped(scene_id, EntityType::EMBEDDED_SCENE)) {
            GetRenderServer()->embedded_scenes.Get(scene_id)->UpdateTurntableCamera(1);
        }

        event_popup::PopupEvents request_close;
        popup_list[i].Draw(&request_close, i*2);
        if (request_close.request_close && !mouse_blocked) {
            // Delete embedded scene
            if (IsIdValidTyped(scene_id, EntityType::EMBEDDED_SCENE)) {
                GetRenderServer()->embedded_scenes.EraseAt(scene_id);
            }
            popup_list.EraseAt(i);
        }
        if (popup_list[i].dragging) {
            popup_list[i].x += GetMouseDelta().x;
            popup_list[i].y += GetMouseDelta().y;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                popup_list[i].dragging = false;
            }
        }
        if (request_close.init_drag && !mouse_blocked) {
            popup_list[i].dragging = true;
        }

        Rectangle blocking_rect = {
            popup_list[i].x,
            popup_list[i].y,
            popup_list[i].width,
            popup_list[i].height + header_height + footer_height,
        };
        if (CheckCollisionPointRec(GetMousePosition(), blocking_rect)) {
            mouse_blocked = true;
        }
    }
}

int event_popup::GetPopupCount() {
    return popup_list.Count();
}

void event_popup::MakeEventPopup(int x, int y, int width, int height, const char* title, 
                                 PopupEvents* events, int relative_offset) {
    total_height = height + header_height + footer_height;
    ui::CreateNew(x, y, width, total_height, DEFAULT_FONT_SIZE, 
                  Palette::ui_main, Palette::bg, z_layers::POPUPS + relative_offset*4);
    ui::Enclose();
    ui::PushInset(header_height);
    
    ui::PushHSplit(0, width - 40);
        ui::WriteEx(title, text_alignment::CENTER, false);
        button_state_flags::T button_state = ui::AsButton();
        // TODO: check if there is nothing above
        events->init_drag = button_state & button_state_flags::JUST_PRESSED;
    ui::Pop();  // HSplit

    ui::PushHSplit(width - 30, width);
        ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 0);
        button_state_flags::T button_state_x = ui::AsButton();
        HandleButtonSound(button_state_x & button_state_flags::JUST_PRESSED);
        events->request_close = button_state_x & button_state_flags::JUST_PRESSED;
        ui::WriteEx(ICON_CROSS, text_alignment::CENTER, false);
    ui::Pop();  // HSplit

    ui::Pop();  // Inset
    draw_state = 1;
    face_height = 0;
}

void event_popup::MakeEventPopupCentered(int width, int height, const char* title, PopupEvents* events) {
    MakeEventPopup((GetScreenWidth() - width) / 2, (GetScreenHeight() - height) / 2,
                   width, height, title, events, 0);
}

void event_popup::EndEventPopup() {
    if (draw_state != 4) {
        FAIL("Invalid operation for this draw state")
    }
    ui::Pop();  // Inset
    ui::Pop();  // Final
    draw_state = 0;
}

void event_popup::EmbeddedSceneFace(RID scene_rid, int height) {
    if (!IsIdValidTyped(scene_rid, EntityType::EMBEDDED_SCENE)) {
        return;
    }
    if (draw_state != 1) {
        FAIL("Cannot start drawing face in this draw state")
    }
    if (height <= 0) return;

    face_height = height;
    ui::PushInset(face_height);
    draw_state = 2;

    EmbeddedScene* scene = GetRenderServer()->embedded_scenes.Get(scene_rid);
    // Only applies the next frame, but still largely forces into complience
    scene->render_width = ui::Current()->width;
    scene->render_height = ui::Current()->height;

    Rectangle source_rect = { 0, 0, scene->render_width, -scene->render_height };  // Render texture is inverted
    ui::Current()->DrawTexture(scene->render_target.texture, source_rect, face_height,
                               WHITE, text_alignment::CENTER, TextBox::TEXTURE_DRAW_RAW);
}

void event_popup::BeginBody() {
    if (draw_state == 1) {
        // Skip header
    } else if (draw_state == 2) {
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
