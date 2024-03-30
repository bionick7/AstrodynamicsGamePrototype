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

void Planet::_UIDrawInventory() {
    const int MARGIN = 3;
    int columns = ui::Current()->width / (SHIP_MODULE_WIDTH + MARGIN);
    int max_rows = (int) std::ceil(MAX_PLANET_INVENTORY / columns);
    int available_height = ui::Current()->height - ui::Current()->y_cursor;
    int height = MinInt(available_height, max_rows * (SHIP_MODULE_HEIGHT + MARGIN));
    int rows = height / (SHIP_MODULE_HEIGHT + MARGIN);
    int i_max = MinInt(rows * columns, MAX_PLANET_INVENTORY);
    ui::PushInset(0, height);
    ui::Current()->width = (SHIP_MODULE_WIDTH + MARGIN) * columns;
    
    ShipModules* sms = GetShipModules();
    for (int i = 0; i < i_max; i++) {
        ShipModuleSlot this_slot = ShipModuleSlot(id, i, ShipModuleSlot::DRAGGING_FROM_PLANET, module_types::ANY);
        //if (!IsIdValid(ship_module_inventory[i])) continue;
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
        sms->DrawShipModule(ship_module_inventory[i]);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Inset
}

void _ProductionQueueMouseHint(RID id, const resource_count_t* planet_resource_array) {
    if (!IsIdValid(id)) return;
    // Assuming monospace font
    int char_width = ui::Current()->GetCharWidth();
    StringBuilder sb;
    const resource_count_t* construction_resources = NULL;
    int build_time = 0;
    int batch_size = 0;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByRID(id);
            ship_class->MouseHintWrite(&sb);
            sb.AutoBreak(ui::Current()->width / char_width);
            ui::Write(sb.c_str);
            sb.Clear();
            construction_resources = &ship_class->construction_resources[0];
            build_time = ship_class->construction_time;
            batch_size = ship_class->construction_batch_size;
            break;
        }
        case EntityType::MODULE_CLASS: {
            const ShipModuleClass* module_class = GetModule(id);
            //sb.Add(module_class->name).Add("\n");
            //sb.Add(module_class->description);
            module_class->MouseHintWrite(&sb);
            sb.AutoBreak(ui::Current()->width / char_width);
            ui::Write(sb.c_str);
            sb.Clear();
            construction_resources = &module_class->construction_resources[0];
            build_time = module_class->GetConstructionTime();
            batch_size = module_class->construction_batch_size;
            break;
        }
        default: break;
    }

    if (construction_resources == NULL) return;
    if (planet_resource_array == NULL) {
        return;
    }

    ui::VSpace(6);
    ui::Write("To build:");
    for (int i=0; i < resources::MAX; i++) {
        if (construction_resources[i] == 0) {
            continue;
        }
        sb.Clear();
        sb.AddFormat("%s: %d  ", GetResourceUIRep(i), construction_resources[i]);
        if (construction_resources[i] <= planet_resource_array[i]) {
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
        } else {
            ui::Current()->text_color = Palette::red;
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
            ui::Current()->text_color = Palette::ui_main;
        }
    }
    ui::Current()->EnsureLineBreak();
    sb.Clear();
    sb.AddFormat("Takes %dD", build_time);
    if(batch_size > 1) {
        sb.AddFormat(" (x%d)", batch_size);
    }
    ui::Write(sb.c_str);
    //if (IsIdValidTyped(id, EntityType::SHIP_CLASS)) {
    //    WireframeMesh wf = assets::GetWirframe(GetShipClassByRID(id)->module_config.mesh_resource_path);
    //}
}

int module_class_ui_tab = 0;
void _UIDrawProduction(Planet* planet, EntityType type) {
    // Set variables for planet/ship
    int option_size;
    List<Planet::ProductionOrder>* queue;
    if (type == EntityType::SHIP_CLASS) {
        option_size = GetShips()->ship_classes_count;
        queue = &planet->ship_production_queue;
    } else if (type == EntityType::MODULE_CLASS) {
        option_size = GetShipModules()->shipmodule_count;
        queue = &planet->module_production_queue;
    }
    int margin = 3;
    int columns = ui::Current()->width / (SHIP_MODULE_WIDTH + margin);
    if (type == EntityType::MODULE_CLASS) {
        columns -= 1;
    }
    int rows = std::ceil(option_size / (double)columns);
    ui::PushInset(0, 50*rows);
    ui::Shrink(5, 5);

    RID hovered_id = GetInvalidId();


    // Draw tabs for modules
    if (type == EntityType::MODULE_CLASS) {
        int panel_width = ui::Current()->width;
        ui::PushHSplit(0, 48);
        int tabs_x = ui::Current()->text_start_x + ui::Current()->width;
        int tabs_y = ui::Current()->text_start_y;
        for(int i=0; i < module_types::MAX; i++) {
            Color tab_draw_color = (module_class_ui_tab == i) ? Palette::ui_main : Palette::ui_alt;
            ui::PushInset(0, 40);
            ui::Shrink(4, 4);
            ui::EncloseEx(0, Palette::bg, tab_draw_color, 0);
            ui::DrawIconSDF(module_types::icons[i], tab_draw_color, 40);
            button_state_flags::T button_state = ui::AsButton();
            HandleButtonSound(button_state);
            if (button_state & button_state_flags::JUST_PRESSED) {
                module_class_ui_tab = i;
            }
            ui::Pop();  // Inset
            ui::HSpace(5);
        }
        ui::Pop(); // HSplit
        ui::PushHSplit(50, panel_width);
    }

    // Draw options

    int draw_index = 0;
    for(int i=0; i < option_size; i++) {
        AtlasPos atlas_pos;
        RID id;
        if (type == EntityType::SHIP_CLASS) {
            id = RID(i, type);
            const ShipClass* sc = GetShipClassByRID(id);
            if (sc->is_hidden) {
                continue;
            }
            atlas_pos = sc->icon_index;
        }
        if (type == EntityType::MODULE_CLASS) {
            id = RID(i, type);
            const ShipModuleClass* smc = GetModule(id);
            if (smc->is_hidden || smc->type != module_class_ui_tab) {
                continue;
            }
            atlas_pos = smc->icon_index;
        }

        //if (!GetTechTree()->IsUnlocked(id)) {
        //    continue;
        //}
        
        if (!planet->CanProduce(id, false, true)) {
            continue;
        }

        ui::PushGridCell(columns, rows, draw_index % columns, draw_index / columns);
        ui::Shrink(margin, margin);
        
        // Possible since Shipclasses get loaded once in continuous mempry
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 4);
            hovered_id = id;
        } else {
            ui::Enclose();
        }
        HandleButtonSound(button_state);
        if ((button_state & button_state_flags::JUST_PRESSED)) {
            Planet::ProductionOrder production_order = planet->MakeProductionOrder(id);
            if (IsIdValid(production_order.worker)) {
                queue->Append(production_order);
            }
        }
        ui::DrawIcon(atlas_pos, Palette::ui_main, ui::Current()->height);
        //ui::Fillline(1.0, Palette::ui_main, Palette::bg);
        //ui::Write(ship_class->description);
        ui::Pop();  // GridCell
        draw_index++;
    }

    if (type == EntityType::MODULE_CLASS) {
        ui::Pop();  // HSplit from tabs
    }

    ui::Pop();  // Inset

    // Draw queue

    bool hover_over_queue = false;
    for(int i=0; i < queue->size; i++) {
        Planet::ProductionOrder production_tuple = queue->Get(i);
        RID id = production_tuple.product;
        Ship* worker = GetShip(production_tuple.worker);

        AtlasPos atlas_pos;
        int total_construction_time;
        if (type == EntityType::SHIP_CLASS) {
            total_construction_time = GetShipClassByRID(id)->construction_time;
            const ShipClass* sc = GetShipClassByRID(id);
            atlas_pos = sc->icon_index;
        } else if (type == EntityType::MODULE_CLASS) {
            total_construction_time = GetShipModules()->GetModuleByRID(id)->construction_time;
            const ShipModuleClass* smc = GetModule(id);
            atlas_pos = smc->icon_index;
        }

        ui::PushInset(0, SHIP_MODULE_HEIGHT + margin + 2);
        ui::Shrink(margin, margin);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::Enclose();
            hovered_id = id;
            hover_over_queue = true;
        } else {
            ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 4);
            ui::Shrink(4, 4);
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            queue->EraseAt(i);

            if (type == EntityType::SHIP_CLASS) {
                worker->ship_production_process = 0;
            } else if (type == EntityType::MODULE_CLASS) {
                worker->module_production_process = 0;
            }
            i--;
        }
        ui::DrawIconSDF(atlas_pos, Palette::ui_main, ui::Current()->height);
        double progress = worker->ship_production_process / (double) total_construction_time;
        if (i == 0) {
            ui::FilllineEx(
                ui::Current()->text_start_x,
                ui::Current()->text_start_x + ui::Current()->width,
                ui::Current()->text_start_y + ui::Current()->height,
                progress, Palette::ui_main, Palette::bg);
        }
        ui::Pop();  // Inset
    }
    if (IsIdValid(hovered_id)) {
        ui::PushMouseHint(GetMousePosition(), 400, 400, 255 - MAX_TOOLTIP_RECURSIONS);
        ui::Current()->text_background = Palette::bg;
        ui::Current()->flexible = true;
        if (hover_over_queue) {
            _ProductionQueueMouseHint(hovered_id, NULL);
        } else {
            _ProductionQueueMouseHint(hovered_id, planet->economy.resource_stock);
        }
        ui::EncloseDynamic(5, Palette::bg, Palette::ui_main, 4);
        ui::Pop();
    }
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

    ui::CreateNew(10, y_start, 340, height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, false);
    ui::Enclose();

    if (IsKeyPressed(KEY_Q)) current_tab = 0;
    //if (IsKeyPressed(KEY_W)) current_tab = 1;
    if (IsKeyPressed(KEY_E)) current_tab = 2;
    if (IsKeyPressed(KEY_A)) current_tab = 3;
    if (IsKeyPressed(KEY_S)) current_tab = 4;
    if (IsKeyPressed(KEY_D)) current_tab = 5;

    ui::PushInset(4, (DEFAULT_FONT_SIZE+4)*2);  // Tab container
    int w = ui::Current()->width;
    const int n_tabs = 6;
    const int tab_columns = 3;
    const char* tab_descriptions[] = {
        "Resources",
        "Economy",
        "Inventory",
        "~Quests~",
        "Ship Production",
        "Module Production",
    };
    static_assert(sizeof(tab_descriptions) / sizeof(tab_descriptions[0]) == n_tabs);

    for (int i=0; i < n_tabs; i++) {
        ui::PushGridCell(tab_columns, 2, i%tab_columns, i/tab_columns);
        //ui::PushHSplit(i * w / n_tabs, (i + 1) * w / n_tabs);
        button_state_flags::T button_state = ui::AsButton();
        HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
        if (i == 1 && !economy.trading_accessible) {
            ui::Write("~Economy~");
            ui::Pop();
            continue;
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            current_tab = i;
        }
        if (button_state & button_state_flags::HOVER || i == current_tab) {
            ui::EnclosePartial(0, Palette::bg, Palette::ui_main, direction::DOWN);
        }
        ui::Write(tab_descriptions[i]);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Tab container
    ui::PushInset(0, 10000);  // Constrained by outside container

    ui::WriteEx(name, text_alignment::CONFORM, true);

    // Independance slider
    ui::PushInset(0, 30);
    Vector2 cursor_pos = ui::Current()->GetTextCursor();
    Rectangle slider_rect = {cursor_pos.x, cursor_pos.y, 300, 20};
    ui::HelperText(GetUI()->GetConceptDescription("independance"));
    if (GetButtonStateRec(slider_rect) & button_state_flags::HOVER) {
        ui::SetMouseHint(independance_delta_log.c_str);
    }
    if (independance > 100) {
        ui::DrawLimitedSlider(independance, 0, 150, 100, 
            300, 20, Palette::red, Palette::ui_alt
        );
    } else {
        ui::DrawLimitedSlider(independance, 0, 150, 100, 
            300, 20, Palette::ui_main, Palette::ui_alt
        );
    }
    StringBuilder sb;
    sb.AddFormat("%3d/%d (%+2d)", independance, 150, independance_delta);
    ui::WriteEx(sb.c_str, text_alignment::CONFORM, true);
    ui::Pop();

    // Opinion slider
    ui::PushInset(0, 30);
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
    ui::Fillline(1, Palette::ui_main, Palette::ui_main);
    ui::Pop();

    ui::Current()->EnsureLineBreak();
    ui::VSpace(10);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(id);
        break;
    case 1:
        economy.UIDrawEconomy(id);
        break;
    case 2:
        _UIDrawInventory();
        break;
    case 3:
        // Quests
        break;
    case 4:
        _UIDrawProduction(this, EntityType::SHIP_CLASS);
        break;
    case 5:
        _UIDrawProduction(this, EntityType::MODULE_CLASS);
        break;
    }

    ui::HelperText(GetUI()->GetConceptDescription("planet"));
    ui::Pop();  // Inset


    // Side ship buttons

    int x_max = ui::Current()->text_start_x + ui::Current()->width;
    ui::PushGlobal(x_max + 10, y_start, 200, 30 * cached_ship_list.size, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, 10);
    for(int i=0; i < cached_ship_list.size; i++) {
        const Ship* ship = GetShip(cached_ship_list[i]);
        ui::PushInset(0, 30 - 8);
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
