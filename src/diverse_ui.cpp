#include "diverse_ui.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "debug_drawing.hpp"
#include "timeline.hpp"
#include "debug_console.hpp"

# define UIPANEL_COUNT 4

bool is_in_pause_menu;
bool is_panel_shown[UIPANEL_COUNT];
Focusables::T current_focus;

const char* panel_icons[] = {
    ICON_QUESTMANAGER,
    ICON_TIMELINE,
    ICON_BATTLELOG,
    ICON_RESEARCHSCREEN
};

const char* panel_descriptions[] = {
    "[DEPRESICATED]",
    "Timeline: Gives an overview of\n ship trajectories",
    "Battlelog: Shows details from\n last battle",
    "Research: Shows conditions \nto unlock new technologies"
};

Focusables::T _ComputeCurrentFocus(GlobalState* gs) {
    for (int i=UIPANEL_COUNT; i >= 0; i--) {
        if (is_panel_shown[i]) {
            return (Focusables::T) i;
        }
    }

    // transfer plan UI
    if (gs->active_transfer_plan.IsActive() || gs->active_transfer_plan.IsSelectingDestination()) {
        return Focusables::TRANSFER_PLAN_UI;
    } 
    
    // cancel out of focused planet and ship
    if (IsIdValid(gs->focused_planet) || IsIdValid(gs->focused_ship)) {
        return Focusables::PLANET_SHIP_DETAILS;
    } 
    return Focusables::MAP;
}

void panel_management::HandleDeselect(GlobalState* gs) {
    current_focus = _ComputeCurrentFocus(gs);

    if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !IsKeyPressed(KEY_ESCAPE)) {
        return;
    }

    // Cancel out of next layer
    PlaySFX(SFX_CANCEL);
    
    switch (current_focus) {
    case Focusables::QUEST_MANAGER:
    case Focusables::TIMELINE:
    case Focusables::COMBAT_LOG:
    case Focusables::TECHTREE:{
        is_panel_shown[current_focus] = false;
        break;}
    case Focusables::TRANSFER_PLAN_UI:{
        gs->active_transfer_plan.Abort();
        break;}
    case Focusables::PLANET_SHIP_DETAILS:{
        gs->focused_planet = GetInvalidId();
        gs->focused_ship = GetInvalidId();
        break;}
    case Focusables::MAP:{
        is_in_pause_menu = !is_in_pause_menu;
        if (is_in_pause_menu) 
            gs->calendar.paused = true;
        break;}
    default: NOT_REACHABLE;
    }
}

void panel_management::DrawUIPanels(GlobalState* gs) {
    // Draw Panels

    if (is_panel_shown[3]) {
        gs->techtree.DrawUI();
    } else if (is_panel_shown[2]) {
        gs->last_battle_log.DrawUI();
    } else if (is_panel_shown[1]) {
        DrawTimeline();
    } else if (is_panel_shown[0]) {
        // Nothing
    }

    // Draw Menu

    const int cell_width = 64;
    const int cell_height = 64;
    const float mouse_reaction_distance = 50.0f;

    int w = cell_width * UIPANEL_COUNT;
    int x = (GetScreenWidth() - w) / 2;
    int y_full = GetScreenHeight() - cell_height - 15;
    int y_none = GetScreenHeight() - 10;
    float mouse_distance = y_full - GetMousePosition().y;
    float mouse_interpolation_factor = 1.0f - Clamp(mouse_distance / mouse_reaction_distance, 0, 1);
    int y = y_none + (int) ((y_full - y_none) * mouse_interpolation_factor);
    ui::CreateNew(x, y, w, cell_height, 40, Palette::ui_main, Palette::bg, z_layers::MENU_PANELS + 1);
    ui::Enclose();
    for (int i=0; i < UIPANEL_COUNT; i++) {
        ui::PushHSplit(i * cell_width, (i+1) * cell_width);

        button_state_flags::T button_state = ui::AsButton();
        if ((button_state & button_state_flags::HOVER) || is_panel_shown[i]) {
            ui::EncloseEx(2, Palette::bg, Palette::interactable_main, 4);
        }
        if (button_state & button_state_flags::HOVER) {
            ui::SetMouseHint(panel_descriptions[i]);
        }
        
        bool is_selected_by_key = !GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_ONE + i);
        if ((button_state & button_state_flags::JUST_PRESSED) || is_selected_by_key) {
            bool is_show_currently = is_panel_shown[i];
            if (GetSetting("make_ui_panels_exclusive")) {
                for (int i=0; i < UIPANEL_COUNT; i++)
                    is_panel_shown[i] = false;        
            }
            is_panel_shown[i] = !is_show_currently;
        }
        HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
        
        ui::WriteEx(panel_icons[i], text_alignment::CENTER, false);

        ui::Pop();  // HSplit
    }
    ui::PushHSplit(0, 50);
    ui::Pop();

    ui::Pop();  // Base (CreateNew)
}

Focusables::T panel_management::GetCurrentFocus() {
    return current_focus;
}

bool panel_management::GetUIPanelVisibility(Focusables::T index) {
    if (index < 0 || index >= UIPANEL_COUNT) return false;
    return is_panel_shown[index];
}

void panel_management::SetUIPanelVisibility(Focusables::T index, bool value) {
    if (index < 0 || index >= UIPANEL_COUNT) return;
    is_panel_shown[index] = value;
}

bool _PauseMenuButton(const char* label) {
    ui::PushInset(DEFAULT_FONT_SIZE+4);
    ui::Enclose();
    ui::Write(label);
    button_state_flags::T button_state = ui::AsButton();
    HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
    ui::Pop();
    return button_state & button_state_flags::JUST_PRESSED;
}

void DrawPauseMenu() {
    if (!is_in_pause_menu) {
        return;
    }

    const int menu_width = 200;
    const int button_height = DEFAULT_FONT_SIZE+10;
    const int menu_height = button_height * 3 + 8;
    ui::CreateNew(
        (GetScreenWidth() - menu_width)/2, 
        (GetScreenHeight() - menu_height)/2, 
        menu_width,
        menu_height,
        DEFAULT_FONT_SIZE,
        Palette::ui_main,
        Palette::bg,
        z_layers::POPUPS + 10
    );
    ui::Enclose();
    if (_PauseMenuButton("Save")) {
        GetGlobalState()->SaveGame("save.yaml");
    }
    if (_PauseMenuButton("Load")) {
        GetGlobalState()->LoadGame("save.yaml");
    }
    if (_PauseMenuButton("Exit")) {
        exit(0);
    }
}
