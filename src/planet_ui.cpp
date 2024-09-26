#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"
#include "logging.hpp"
#include "string_builder.hpp"
#include "render_utils.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"

void Planet::_UIDrawInventory() {
    const int MARGIN = 3;
    int columns = ui::Current()->width / (SHIP_MODULE_WIDTH + MARGIN);
    int max_rows = (int) std::ceil(MAX_PLANET_INVENTORY / columns);
    int available_height = ui::Current()->height - ui::Current()->y_cursor;
    int height = MinInt(available_height, max_rows * (SHIP_MODULE_HEIGHT + MARGIN));
    int rows = height / (SHIP_MODULE_HEIGHT + MARGIN);
    int i_max = MinInt(rows * columns, MAX_PLANET_INVENTORY);
    ui::PushInset(height);
    ui::Current()->width = (SHIP_MODULE_WIDTH + MARGIN) * columns;
    
    ShipModules* sms = GetShipModules();
    for (int i = 0; i < i_max; i++) {
        ShipModuleSlot this_slot = ShipModuleSlot(id, i, ShipModuleSlot::DRAGGING_FROM_PLANET, module_types::ANY);
        //if (!IsIdValid(inventory[i])) continue;
        ui::PushGridCell(columns, rows, i % columns, i / columns);
        ui::Shrink(MARGIN, MARGIN);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            current_slot = this_slot;
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                sms->DirectSwap(current_slot);
            } else {
                sms->InitDragging(current_slot, ui::Current()->render_rec);
            }
        }
        this_slot.Draw();
        ui::EncloseEx(0, BLANK, Palette::ui_alt, 3);
        //ui::BeginDirectDraw();
        //DrawRectangleLinesEx(ui::Current()->GetRect(), 1, Palette::ui_alt);
        //ui::EndDirectDraw();
        sms->DrawShipModule(inventory[i], false);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Inset
}

int current_tab = 0;  // Global variable, I suppose
void Planet::DrawUI() {
    Vector2 screen_pos = GetCamera()->GetScreenPos(position.cartesian);
    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    if (mouse_hover) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 20, Palette::ui_main);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }

    //          True UI
    // ============================

    // Reset
    current_slot = ShipModuleSlot();

    int y_start = -1;
    int height = -1;

    const TransferPlan* tp = GetTransferPlanUI()->plan;

    if (GetTransferPlanUI()->IsActive()){
        if (tp->departure_planet == id) {
            y_start = 10;
            height = GetScreenHeight() / 2 - 20;
            //transfer = tp->resource_transfer.Inverted();
            //fuel_draw = tp->fuel;
        } else if (tp->arrival_planet == id) {
            y_start = GetScreenHeight() / 2 + 10;
            height = GetScreenHeight() / 2 - 20;
            //transfer = tp->resource_transfer;
        } else {
            return;
        }
    } else if (mouse_hover || 
        (GetGlobalState()->focused_planet == id && !IsIdValidTyped(GetGlobalState()->hover, EntityType::PLANET))
    ) {
        y_start = 10;
        height = GetScreenHeight() - 20;
    } else {
        return;
    }

    ui::CreateNew(10, y_start, 340, height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE);
    ui::Enclose();

    if (IsKeyPressed(KEY_Q)) current_tab = 0;
    //if (IsKeyPressed(KEY_W)) current_tab = 1;
    if (IsKeyPressed(KEY_E)) current_tab = 2;
    if (IsKeyPressed(KEY_A)) current_tab = 3;
    if (IsKeyPressed(KEY_S)) current_tab = 4;
    if (IsKeyPressed(KEY_D)) current_tab = 5;

    ui::PushInset((DEFAULT_FONT_SIZE+4)*2);  // Tab container
    int w = ui::Current()->width;
    const int n_tabs = 2;
    const int tab_columns = 2;
    const char* tab_descriptions[] = {
        "Resources",
        "Inventory",
        //"Economy",
        //"~Quests~",
        //"Ship Production",
        //"Module Production",
    };
    static_assert(sizeof(tab_descriptions) / sizeof(tab_descriptions[0]) == n_tabs);

    for (int i=0; i < n_tabs; i++) {
        ui::PushGridCell(tab_columns, 1, i%tab_columns, i/tab_columns);
        //ui::PushHSplit(i * w / n_tabs, (i + 1) * w / n_tabs);
        button_state_flags::T button_state = ui::AsButton();
        HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
        //if (i == 1 && !economy.trading_accessible) {
        //    ui::Write("~Economy~");
        //    ui::Pop();
        //    continue;
        //}
        if (button_state & button_state_flags::JUST_PRESSED) {
            current_tab = i;
        }if (button_state & button_state_flags::HOVER || i == current_tab) {
            ui::EnclosePartial(0, Palette::bg, Palette::ui_main, direction::DOWN);
        }
        ui::Write(tab_descriptions[i]);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Tab container
    ui::PushInset(10000);  // Constrained by outside container

    ui::WriteEx(name.GetChar(), text_alignment::CONFORM, true);

    // Independence slider
    /*ui::PushInset(30);
    Vector2 cursor_pos = ui::Current()->GetTextCursor();
    Rectangle slider_rect = {cursor_pos.x, cursor_pos.y, 300, 20};
    ui::HelperText(GetUI()->GetConceptDescription("independence"));
    if (GetButtonStateRec(slider_rect) & button_state_flags::HOVER) {
        ui::SetMouseHint(independence_delta_log.c_str);
    }
    if (independence > 100) {
        ui::DrawLimitedSlider(independence, 0, 150, 100, 
            300, 20, Palette::red, Palette::ui_alt
        );
    } else {
        ui::DrawLimitedSlider(independence, 0, 150, 100, 
            300, 20, Palette::ui_main, Palette::ui_alt
        );
    }
    StringBuilder sb;
    sb.AddFormat("%3d/%d (%+2d)", independence, 150, independence_delta);
    ui::WriteEx(sb.c_str, text_alignment::CONFORM, true);
    ui::Pop();*/

    // Opinion slider
    /*ui::PushInset(30);
    cursor_pos = ui::Current()->GetTextCursor();
    slider_rect = {cursor_pos.x, cursor_pos.y, 300, 20};
    ui::HelperText(GetUI()->GetConceptDescription("opinion"));
    if (GetButtonStateRec(slider_rect) & button_state_flags::HOVER) {
        ui::SetMouseHint(opinion_delta_log.c_str);
    }
    if (opinion >= 0) {
        ui::DrawLimitedSlider(opinion, -100, 100, 0, 
            300, 20, Palette::red, Palette::ui_alt
        );
    } else {
        ui::DrawLimitedSlider(opinion, -100, 100, 0, 
            300, 20, Palette::ui_main, Palette::ui_alt
        );
    }
    sb.Clear();
    sb.AddFormat("%3d (%+2d)", opinion, opinion_delta);
    ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
    ui::FillLine(1, Palette::ui_main, Palette::ui_main);
    ui::Pop();*/

    ui::Current()->EnsureLineBreak();
    ui::VSpace(10);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(id);
        break;
    case 1:
        _UIDrawInventory();
        break;
    }

    ui::HelperText(GetUI()->GetConceptDescription("planet"));
    ui::Pop();  // Inset

    // Side ship buttons

    int x_max = ui::Current()->x + ui::Current()->width;
    ui::PushGlobal(x_max + 10, y_start, 200, 30 * cached_ship_list.size, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, 10);
    for(int i=0; i < cached_ship_list.size; i++) {
        const Ship* ship = GetShip(cached_ship_list[i]);
        ui::PushInset(30);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::JUST_PRESSED) {
            GetGlobalState()->focused_ship = cached_ship_list[i];
        }
        if (button_state & button_state_flags::HOVER) {
            ui::Current()->text_color = Palette::ui_main;
        } else {
            ui::Current()->text_color = ship->GetColor();
        }
        HandleButtonSound(button_state);
        ui::WriteEx(ship->GetTypeIcon(), text_alignment::CONFORM, false);
        ui::Write(ship->name);
        ui::Pop();
    }
    ui::Pop();  // Global
}

void SetPlanetTabIndex(int index) {
    if (index >= 0 && index < 2) {
        current_tab = index;
    }
}
