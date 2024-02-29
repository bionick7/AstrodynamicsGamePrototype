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
        //if (!IsIdValid(ship_module_inventory[i])) continue;
        ui::PushGridCell(columns, rows, i % columns, i / columns);
        ui::Shrink(MARGIN, MARGIN);
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            current_slot = ShipModuleSlot(id, i, ShipModuleSlot::DRAGGING_FROM_PLANET);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                sms->DirectSwap(current_slot);
            } else {
                sms->InitDragging(current_slot, ui::Current()->render_rec);
            }
        }
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
            sb.Add(ship_class->name).Add("\n");
            sb.Add(ship_class->description);
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
            construction_resources = &module_class->construction_resources[0];
            build_time = module_class->GetConstructionTime();
            batch_size = module_class->construction_batch_size;
            module_class->MouseHintWrite();
            break;
        }
        default: break;
    }

    if (construction_resources == NULL) return;

    if (planet_resource_array == NULL) {
        return;
    }
    ui::VSpace(10);
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (construction_resources[i] == 0) {
            continue;
        }
        sb.Clear();
        sb.AddFormat("%s: %d", GetResourceUIRep(i), construction_resources[i]);
        if (construction_resources[i] <= planet_resource_array[i]) {
            ui::Write(sb.c_str);
        } else {
            ui::Current()->text_color = Palette::red;
            ui::Write(sb.c_str);
            ui::Current()->text_color = Palette::ui_main;
        }
    }
    sb.Clear();
    sb.AddI(build_time).Add("D");
    if(batch_size > 1) {
        sb.AddFormat(" (x%d)", batch_size);
    }
    ui::Write(sb.c_str);
}

void _UIDrawProduction(const Planet* planet, int option_size, IDList* queue, double progress,
    RID id_getter(int), AtlasPos icon_getter(RID), bool include_getter(RID)
) {
    // Draw options
    int margin = 3;
    int columns = ui::Current()->width / (SHIP_MODULE_WIDTH + margin);
    int rows = std::ceil(option_size / (double)columns);
    ui::PushInset(0, 50*rows);
    ui::Shrink(5, 5);

    RID hovered_id = GetInvalidId();
    for(int i=0; i < option_size; i++) {
        RID id = id_getter(i);
        if (!include_getter(id)) {
            continue;
        }
        ui::PushGridCell(columns, rows, i % columns, i / columns);
        ui::Shrink(margin, margin);
        
        // Possible since Shipclasses get loaded once in continuous mempry
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            ui::EncloseEx(4, Palette::bg, Palette::interactable_main, 4);
            hovered_id = id;
        } else {
            ui::Enclose();
        }
        HandleButtonSound(button_state);
        if ((button_state & ButtonStateFlags::JUST_PRESSED) && planet->CanProduce(id)) {
            queue->Append(id);
        }

        ui::DrawIcon(icon_getter(id), Palette::ui_main, ui::Current()->height);
        //ui::Fillline(1.0, Palette::ui_main, Palette::bg);
        //ui::Write(ship_class->description);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Inset

    // Draw queue
    bool hover_over_queue = false;
    for(int i=0; i < queue->size; i++) {
        RID id = queue->Get(i);
        ui::PushInset(0, SHIP_MODULE_HEIGHT + margin +2);
        ui::Shrink(margin, margin);
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            ui::Enclose();
            hovered_id = id;
            hover_over_queue = true;
        } else {
            ui::EncloseEx(4, Palette::bg, Palette::interactable_main, 4);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            queue->EraseAt(i);
            i--;
        }
        ui::DrawIconSDF(icon_getter(id), Palette::ui_main, ui::Current()->height);
        if (i == 0) {
            ui::Fillline(progress, Palette::ui_main, Palette::bg);
        }
        _ProductionQueueMouseHint(id, NULL);
        ui::Pop();  // Inset
    }
    if (IsIdValid(hovered_id)) {
        ui::PushMouseHint(GetMousePosition(), 400, 400, 255 - MAX_TOOLTIP_RECURSIONS);
        ui::Enclose();
        if (hover_over_queue) {
            _ProductionQueueMouseHint(hovered_id, NULL);
        } else {
            _ProductionQueueMouseHint(hovered_id, planet->economy.resource_stock);
        }
        ui::Pop();
    }
}

void Planet::_UIDrawModuleProduction() {
    double progress = 0.0;
    if (module_production_queue.Count() > 0) {
        const ShipModuleClass* module_class = GetModule(module_production_queue[0]);
        progress = Clamp(module_production_process / (float)module_class->GetConstructionTime(), 0.0f, 1.0f);
    }
    _UIDrawProduction(
        this, GetShipModules()->shipmodule_count,
        &module_production_queue, progress,
        [](int i) { return RID(i, EntityType::MODULE_CLASS); },
        [](RID id) { return GetModule(id)->icon_index; },
        [](RID id) { return !GetModule(id)->is_hidden; }
    );
}

void Planet::_UIDrawShipProduction() {
    double progress = 0.0;
    if (ship_production_queue.Count() > 0) {
        const ShipClass* ship_class = GetShipClassByRID(ship_production_queue[0]);
        progress = Clamp(ship_production_process / (float)ship_class->construction_time, 0.0f, 1.0f);
    }
    _UIDrawProduction(
        this, GetShips()->ship_classes_count,
        &ship_production_queue, progress,
        [](int i) { return RID(i, EntityType::SHIP_CLASS); },
        [](RID id) { return GetShipClassByRID(id)->icon_index; },
        [](RID id) { return !GetShipClassByRID(id)->is_hidden; }
    );
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
    ResourceTransfer transfer = ResourceTransfer();
    ResourceTransfer fuel_draw = ResourceTransfer();

    const TransferPlan* tp = GetTransferPlanUI()->plan;

    if (GetTransferPlanUI()->IsActive()){
        if (tp->departure_planet == id) {
            y_start = 10;
            height = GetScreenHeight() / 2 - 20;
            transfer = tp->resource_transfer.Inverted();
            fuel_draw = tp->fuel;
        } else if (tp->arrival_planet == id) {
            y_start = GetScreenHeight() / 2 + 10;
            height = GetScreenHeight() / 2 - 20;
            transfer = tp->resource_transfer;
        } else {
            return;
        }
    } else if (mouse_hover || 
        (GetGlobalState()->focused_planet == id && !IsIdValidTyped(GetGlobalState()->hover, EntityType::PLANET))
    ) {
        y_start = 10;
        height = GetScreenHeight() - 20;
        transfer = ResourceTransfer();
        fuel_draw = ResourceTransfer();
    } else {
        return;
    }


    ui::CreateNew(10, y_start, 340, height, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg);
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
        ButtonStateFlags::T button_state = ui::AsButton();
        HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
        if (i == 1 && !economy.trading_accessible) {
            ui::Write("~Economy~");
            ui::Pop();
            continue;
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            current_tab = i;
        }
        if (button_state & ButtonStateFlags::HOVER || i == current_tab) {
            ui::EnclosePartial(0, Palette::bg, Palette::ui_main, Direction::DOWN);
        }
        ui::Write(tab_descriptions[i]);
        ui::Pop();  // GridCell
    }
    ui::Pop();  // Tab container
    ui::PushInset(0, 10000);  // Constrained by outside container

    ui::WriteEx(name, TextAlignment::CONFORM, false);
    ui::Fillline(1, Palette::ui_main, Palette::ui_main);
    ui::VSpace(5);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(transfer, fuel_draw);
        break;
    case 1:
        economy.UIDrawEconomy(transfer, fuel_draw);
        break;
    case 2:
        _UIDrawInventory();
        break;
    case 3:
        // Quests
        break;
    case 4:
        _UIDrawShipProduction();
        break;
    case 5:
        _UIDrawModuleProduction();
        break;
    }

    ui::HelperText(GetUI()->GetConceptDescription("planet"));
    ui::Pop();  // Inset


    // Side ship buttons

    IDList ships_around_planet = IDList();
    GetShips()->GetOnPlanet(&ships_around_planet, id, UINT32_MAX);

    int x_max = ui::Current()->text_start_x + ui::Current()->width;
    ui::PushGlobal(x_max + 10, y_start, 200, 30 * ships_around_planet.size, DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, 10);
    for(int i=0; i < ships_around_planet.size; i++) {
        const Ship* ship = GetShip(ships_around_planet[i]);
        ui::PushInset(0, 30-8);
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            GetGlobalState()->focused_ship = ships_around_planet[i];
        }
        if (button_state & ButtonStateFlags::HOVER) {
            ui::Current()->text_color = Palette::ui_main;
        } else {
            ui::Current()->text_color = ship->GetColor();
        }
        HandleButtonSound(button_state);
        ui::WriteEx(ship->GetTypeIcon(), TextAlignment::CONFORM, false);
        ui::Write(ship->name);
        ui::Pop();
    }
    ui::Pop();  // Global
}
