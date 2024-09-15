#include "ship.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"
#include "utils.hpp"

void _UIDrawHeader(const Ship* ship) {
    ui::WriteEx(ship->name, text_alignment::CONFORM, false);  
    ui::WriteEx(ship->GetTypeIcon(), text_alignment::CONFORM, false);
    const char* text = ICON_EMPTY " " ICON_EMPTY "  ";  // use ICON_EMPTY instead of space to get the right spacing
    if (!ship->IsParked() && !ship->IsLeading()) text = ICON_EMPTY " " ICON_TRANSPORT_FLEET "  ";
    if (ship->IsParked() && ship->IsLeading()) text = ICON_PLANET " " ICON_EMPTY "  ";
    if (ship->IsParked() && !ship->IsLeading()) text = ICON_PLANET " " ICON_TRANSPORT_FLEET "  ";
    text::Layout layout = ui::Current()->GetTextLayout(text, text_alignment::RIGHT | text_alignment::VCONFORM);
    int pos = layout.GetCharacterIndex(GetMousePosition());
    ui::WriteEx(text, text_alignment::RIGHT | text_alignment::VCONFORM, true);
    button_state_flags::T planet_button_state = GetButtonState(pos >= 0 && pos < 2, false);  // Don't care about hover_in
    button_state_flags::T fleet_button_state = GetButtonState(pos >= 3 && pos < 5, false);  // Don't care about hover_in
    if (planet_button_state & button_state_flags::JUST_PRESSED && ship->IsParked()) {
        GetGlobalState()->focused_planet = ship->GetParentPlanet();
    }
    if (fleet_button_state & button_state_flags::JUST_PRESSED && !ship->IsLeading()) {
        GetGlobalState()->focused_ship = ship->parent_obj;
    }
    
    if (planet_button_state & button_state_flags::HOVER && ship->IsParked()) {
        ui::SetMouseHint("Select parent planet");
    }
    if (fleet_button_state & button_state_flags::HOVER && !ship->IsLeading()) {
        ui::SetMouseHint("Select leading ship");
    }
    ui::VSpace(10);
}

int ship_stats_current_tab = 0;

void _UIDrawStats(const Ship* ship) {
    const int combat_stat_block_height = 4 * 20;
    const int industry_stat_block_height = 3 * 20;

    // Power
    if (ship->power() > 0) {
        ui::WriteEx(
            TextFormat(ICON_POWER " %d / ?? ", ship->power()), 
            text_alignment::LEFT | text_alignment::VCONFORM, false
        );
    }
    // Initiative
    if (ship->initiative() > 0) {
        ui::WriteEx(
            TextFormat(ICON_INITIATIVE " %d ", ship->initiative()), 
            text_alignment::RIGHT | text_alignment::VCONFORM, false
        );
    }
    if (ship->power() + ship->initiative() > 0) {
        ui::Write("");  // Linebreak
    }

    // Payload
    ui::WriteEx(
        TextFormat(ICON_PAYLOAD " %d / %d ", KGToResourceCounts(ship->GetOperationalMass()), ship->GetMaxCapacity()), 
        text_alignment::LEFT | text_alignment::VCONFORM, false
    );
    // DV
    if (!ship->IsStatic()) {
        ui::WriteEx(
            TextFormat("\u0394V %2.2f km/s  ", ship->GetCapableDV() / 1000.f), 
            text_alignment::RIGHT | text_alignment::VCONFORM, false
        );
    }
    ui::Current()->EnsureLineBreak();
    ui::VSpace(5);

    // Tabs
    int width = ui::Current()->width;
    ui::PushInset(25);
    for (int i=0; i < 2; i++) {
        if (i == 0) ui::PushHSplit(5, width/2);
        else ui::PushHSplit(width/2, width - 5);
            button_state_flags::T button_state = ui::AsButton();
            HandleButtonSound(button_state);
            if (button_state & button_state_flags::JUST_PRESSED) ship_stats_current_tab = i;
            if (button_state & button_state_flags::HOVER || ship_stats_current_tab == i) 
                ui::EnclosePartial(0, Palette::bg, Palette::ui_main, direction::DOWN);
            else
                ui::EnclosePartial(0, Palette::bg, Palette::ui_alt, direction::DOWN);
            if (i == 0) ui::WriteEx("Production", text_alignment::VCENTER | text_alignment::LEFT, false);
            else if (GetTechTree()->IsMilestoneReached("combat")) ui::WriteEx("Combat", text_alignment::VCENTER | text_alignment::RIGHT, false);
        ui::Pop();
    }
    ui::Pop();

    ui::FillLine(ship->GetOperationalMass() / ResourceCountsToKG(ship->GetMaxCapacity()), Palette::ui_main, Palette::bg);
    ui::VSpace(15);
    StringBuilder sb;
    if (ship_stats_current_tab == 0) {
        ui::PushInset(industry_stat_block_height);

        int count = 0;
        for(int i=ship_stats::GROUND_CONNECTION; i < ship_stats::MAX; i++) {
            if (ship->stats[i] != 0) {
                const char* text = TextFormat("%s %d ", ship_stats::icons[i], ship->stats[i]);
                text::Layout layout = ui::Current()->GetTextLayout(text, text_alignment::CONFORM);
                ui::Current()->WriteLayout(&layout, true);
                if (count++ % 6 == 5) ui::Current()->LineBreak();
                int char_index = layout.GetCharacterIndex(GetMousePosition());
                if (char_index >= 0 && char_index < 2) {
                    ui::SetMouseHint(ship_stats::names[i]);
                }
            }
        }

        ui::HelperText(GetUI()->GetConceptDescription("stat"));
        ui::Pop();  // Inset
    } else {
        ui::PushInset(combat_stat_block_height);
        //sb.AddFormat(ICON_POWER "%2d" ICON_ACS "%2d\n", ship->power(), ship->initiative());
        sb.AddFormat(" %2d/%2d" ICON_HEART_KINETIC "  %2d/%2d" ICON_HEART_ENERGY "  %3d/%3d" ICON_HEART_BOARDING "\n", 
                    ship->kinetic_hp() - ship->dammage_taken[ship_variables::KINETIC_ARMOR], ship->kinetic_hp(),
                    ship->energy_hp()  - ship->dammage_taken[ship_variables::ENERGY_ARMOR],  ship->energy_hp(),
                    ship->crew()       - ship->dammage_taken[ship_variables::CREW],          ship->crew());
        sb.AddFormat("   %2d " ICON_ATTACK_KINETIC "    %2d " ICON_ATTACK_ORDNANCE "     %3d " ICON_ATTACK_BOARDING "\n", 
                    ship->kinetic_offense(), ship->ordnance_offense(), ship->boarding_offense());
        sb.AddFormat("   %2d " ICON_SHIELD_KINETIC "    %2d " ICON_SHIELD_ORDNANCE "     %3d " ICON_SHIELD_BOARDING "\n", 
                    ship->kinetic_defense(), ship->ordnance_defense(), ship->boarding_defense());
        //sb.AddFormat("++++++++++++++++++");
        ui::Write(sb.c_str);
        ui::HelperText(GetUI()->GetConceptDescription("stat"));
        ui::Pop();  // Inset
    }
}

int _UIDrawTransferplans(Ship* ship) {
    int inset_height = ship->prepared_plans_count * (ui::Current()->GetLineHeight() * 2 + 8 + 4);
    inset_height += 40;

    ui::PushInset(inset_height);
    ui::Enclose();
    timemath::Time now = GlobalGetNow();
    for (int i=0; i < ship->prepared_plans_count; i++) {
        char tp_str[2][40];
        const char* resource_name;
        const char* departure_planet_name;
        const char* arrival_planet_name;

        resource_name = "EMPTY";
        resource_count_t dominant_rsc = 0;
        for (int j=0; j < resources::MAX; j++) {
            if (ship->prepared_plans[i].resource_transfer[j] > dominant_rsc) {
                dominant_rsc = ship->prepared_plans[i].resource_transfer[j];
                resource_name = resources::names[j];
            }
        }
        if (IsIdValid(ship->prepared_plans[i].departure_planet)) {
            departure_planet_name = GetPlanet(ship->prepared_plans[i].departure_planet)->name.GetChar();
        } else {
            departure_planet_name = "NOT SET";
        }
        if (IsIdValid(ship->prepared_plans[i].arrival_planet)) {
            arrival_planet_name = GetPlanet(ship->prepared_plans[i].arrival_planet)->name.GetChar();
        } else {
            arrival_planet_name = "NOT SET";
        }

        StringBuilder sb;
        sb.AddFormat(
            "- %s (%3d D %2d H)\n", resource_name,
            (int) (ship->prepared_plans[i].arrival_time - now).Seconds() / timemath::SECONDS_IN_DAY,
            ((int) (ship->prepared_plans[i].arrival_time - now).Seconds() % timemath::SECONDS_IN_DAY) / 3600
        );
        sb.AddFormat("  %s >> %s", departure_planet_name, arrival_planet_name);

        // Double Button
        ui::PushInset(ui::Current()->GetLineHeight() * 2 + 8);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EnclosePartial(4, Palette::bg, Palette::interactable_main, direction::DOWN | direction::LEFT);
        } else {
            ui::EnclosePartial(4, Palette::bg, Palette::ui_main, direction::DOWN | direction::LEFT);
        }
        ui::PushHSplit(0, -32);
        ui::Write(sb.c_str);
        HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
        if (button_state & button_state_flags::HOVER) {
            ship->highlighted_plan_index = i;
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            ship->StartEditingPlan(i);
        }
        ui::Pop();  // HSplit

        if (i != ship->plan_edit_index) {
            ui::PushHSplit(-32, -1);
            button_state_flags::T button_state = ui::AsButton();
            if (button_state & button_state_flags::HOVER) {
                ui::EncloseEx(4, Palette::interactable_alt, Palette::interactable_alt, 4);
            } else {
                ui::EncloseEx(4, Palette::bg, Palette::bg, 4);
            }
            ui::WriteEx(ICON_X, text_alignment::CENTER, false);
            HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
            if (button_state & button_state_flags::JUST_PRESSED) {
                ship->RemoveTransferPlan(i);
            }
            ui::Pop();  // HSplit
        }

        ui::Pop();  // Insert
    }
    ui::HelperText(GetUI()->GetConceptDescription("transfer"));
    
    // 'New transfer' button

    ui::VSpace(10);
    Rectangle text_rect = ui::Current()->TbMeasureText("New transfer", text_alignment::CONFORM);
    //ui::PushInline(text_rect.width + 2, text_rect.height + 2);
    ui::PushAligned(text_rect.width + 6, text_rect.height + 4, text_alignment::HCENTER | text_alignment::VCONFORM);
        button_state_flags::T new_button_state = ui::AsButton();
        if (new_button_state & button_state_flags::HOVER) {
            ui::EnclosePartial(0, Palette::bg, Palette::interactable_main, direction::DOWN);
        } else {
            ui::EnclosePartial(0, Palette::bg, Palette::ui_main, direction::DOWN);
        }
        ui::WriteEx("New transfer", text_alignment::CENTER, false);
        HandleButtonSound(new_button_state);
    ui::Pop();  // Aligned
    ui::VSpace(text_rect.height + 4);
    if (new_button_state & button_state_flags::JUST_PRESSED) {
        ship->_OnNewPlanClicked();
    }
    ui::Pop();  // Inset

    return inset_height;
}

void _UIDrawFleet(Ship* ship) {
    // Following
    if (!ship->IsLeading()) {
        ui::PushInset(DEFAULT_FONT_SIZE+4);
        StringBuilder sb;
        if (ship->IsParked()) {
            sb.Add("Detach from").Add(GetShip(ship->parent_obj)->name);
            button_state_flags::T button_state = ui::DirectButton(sb.c_str, 0);
            HandleButtonSound(button_state);
            if (button_state & button_state_flags::JUST_PRESSED) {
                ship->Detach();
            }
        } else {
            sb.Add("Following").Add(GetShip(ship->parent_obj)->name);
            ui::Write(sb.c_str);
        }
        ui::Pop();  // Inset
        return;
    }
    IDList candidates;
    ship_selection_flags::T selection_flags = ship_selection_flags::GetAllegianceFlags(ship->allegiance);
    GetShips()->GetOnPlanet(&candidates, ship->GetParentPlanet(), selection_flags);
    for (int i=0; i < candidates.size; i++) {
        if (candidates[i] == ship->id) continue;
        const Ship* candidate = GetShip(candidates[i]);
        if (!candidate->IsLeading()) continue;
        ui::PushInset(DEFAULT_FONT_SIZE + 4);
        ui::WriteEx("Attach to ", text_alignment::CONFORM, false);

        ui::Current()->text_color = Palette::interactable_main;
        ui::Write(candidate->name);
        ui::Current()->text_color = Palette::ui_main;

        button_state_flags::T button_state = ui::AsButton();
        HandleButtonSound(button_state);
        if (button_state & button_state_flags::JUST_PRESSED) {
            ship->AttachTo(candidate->id);
        }
        ui::Pop();  // Inset
    }
}

void _UIDrawQuests(Ship* ship) {
    QuestManager* qm = GetQuestManager();
    double max_mass = ResourceCountsToKG(ship->GetMaxCapacity()) - ship->GetOperationalMass();
    
    for(auto it = qm->active_tasks.GetIter(); it; it++) {
        Task* quest = qm->active_tasks.Get(it);
        bool is_quest_in_cargo = quest->ship == ship->id;
        if (quest->current_planet != ship->parent_obj && !is_quest_in_cargo) continue;
        bool can_accept = quest->payload_mass <= max_mass;
        button_state_flags::T button_state = quest->DrawUI(true, is_quest_in_cargo);
        if (button_state & button_state_flags::JUST_PRESSED && can_accept) {
            if (is_quest_in_cargo) {
                qm->PutbackTask(ship->id, it.GetId());
            } else {
                qm->PickupTask(ship->id, it.GetId());
            }
        }
    }
}

void _ProductionQueueMouseHint(RID id, const Ship* ship, bool is_in_production_queue) {
    if (!IsIdValid(id)) return;
    // Assuming monospace font
    // TODO: ^ Assumption is no longer true
    
    const Planet* planet = GetPlanet(ship->GetParentPlanet());

    int char_width = ui::Current()->GetCharWidth();
    StringBuilder sb;
    const resource_count_t* construction_resources = NULL;
    const int* construction_requirements = NULL;
    int build_time = 0;
    int batch_size = 0;
    bool is_planet_invalid = false;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByRID(id);
            ship_class->MouseHintWrite(&sb);
            sb.AutoBreak(ui::Current()->width / char_width - 10);
            construction_resources = &ship_class->construction_resources[0];
            construction_requirements =  &ship_class->construction_requirements[0];
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
            construction_resources = &module_class->construction_resources[0];
            construction_requirements =  &module_class->construction_requirements[0];
            build_time = module_class->GetConstructionTime();
            batch_size = module_class->construction_batch_size;
            is_planet_invalid = module_class->IsPlanetRestricted(planet->id);
            break;
        }
        default: break;
    }
    
    // Adjust textbox if text doesn't fit
    text::Layout description_layout = ui::Current()->GetTextLayout(sb.c_str, text_alignment::CONFORM);
    int overshoot = description_layout.bounding_box.x + description_layout.bounding_box.width - GetScreenWidth();
    if (overshoot > 0) {
        description_layout.Offset(-overshoot, 0);
    }
    if (ui::Current()->x > description_layout.bounding_box.x) {
        ui::Current()->x = description_layout.bounding_box.x;
    }
    ui::Current()->WriteLayout(&description_layout, true);
    sb.Clear();

    ui::Current()->LineBreak();

    if (construction_resources == NULL) return;
    if (construction_requirements == NULL) return;

    // Build requirements

    ui::VSpace(6);
    ui::Write("To build:");

    // Resources
    for (int i=0; i < resources::MAX; i++) {
        if (construction_resources[i] == 0) {
            continue;
        }
        sb.Clear();
        sb.AddFormat("%s: %d  ", GetResourceUIRep(i), construction_resources[i]);
        
        if (construction_resources[i] <= planet->economy.resource_stock[i]) {
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
        } else {
            ui::Current()->text_color = Palette::red;
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
            ui::Current()->text_color = Palette::ui_main;
        }
    }

    ui::Current()->LineBreak();

    if (is_planet_invalid) {
        ui::Current()->text_color = Palette::red;
        ui::Write("Not working on this planet");
        ui::Current()->text_color = Palette::ui_main;
    }

    // Stats
    for (int i=0; i < ship_stats::MAX; i++) {
        if (construction_requirements[i] == 0) {
            continue;
        }
        sb.Clear();
        sb.AddFormat("%s: %d  ", ship_stats::icons[i], construction_requirements[i]);
        if (construction_requirements[i] <= ship->stats[i]) {
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
        } else {
            ui::Current()->text_color = Palette::red;
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
            ui::Current()->text_color = Palette::ui_main;
        }
    }
    
    ui::Current()->LineBreak();

    sb.Clear();
    
    if (is_in_production_queue) {
        sb.AddFormat("Productions: %d/%dD", ship->production_process, build_time);
    } else {
        sb.AddFormat("Takes %dD", build_time);
    }
    if(batch_size > 1) {
        sb.AddFormat(" (x%d)", batch_size);
    }
    ui::Write(sb.c_str);
    //if (IsIdValidTyped(id, EntityType::SHIP_CLASS)) {
    //    WireframeMesh wf = assets::GetWireframe(GetShipClassByRID(id)->module_config.mesh_resource_path);
    //}
}

int module_class_ui_tab = 0;
void _UIDrawProduction(Ship* ship) {
    if (ship == NULL) {
        return;
    }
    if (!ship->IsParked()) {
        return;
    }

    const int tabs = module_types::ANY + 1;

    // Set variables for planet/ship
    int option_size = GetShips()->ship_classes_count + GetShipModules()->shipmodule_count;
    int option_sizes[tabs] = {0};
    IDList options[tabs];
    for (int i=0; i < tabs; i++) options[i].Clear();

    // Compile tab list
    for (int i=0; i < option_size; i++) {
        RID id;
        int options_index = 0;
        if (i < GetShipModules()->shipmodule_count) {
            id = RID(i, EntityType::MODULE_CLASS);
            const ShipModuleClass* smc = GetModule(id);
            if (smc->is_hidden) {
                continue;
            }
            options_index = smc->type;
        } else {
            id = RID(i - GetShipModules()->shipmodule_count, EntityType::SHIP_CLASS);
            const ShipClass* sc = GetShipClassByRID(id);
            if (sc->is_hidden) {
                continue;
            }
            options_index = tabs-1;
        }
        if (!GetTechTree()->IsUnlocked(id)) {
            continue;
        }
        //if (!ship->CanProduce(id, false, true) && !GetSettingBool("show_unconstructable_products")) {
        //    continue;
        //}
        options[options_index].Append(id);
    }

    int max_display_options = 0;
    for (int i=0; i < tabs; i++) {
        if (options[i].size > max_display_options) {
            max_display_options = options[i].size;
        }
    }
    
    int margin = 3;
    int columns = ui::Current()->width / (SHIP_MODULE_WIDTH + margin) - 1;
    int rows = std::ceil(max_display_options / (double)columns);
    int height = MaxInt((SHIP_MODULE_HEIGHT + margin)*rows, 50*tabs);
    rows = height / (SHIP_MODULE_HEIGHT + margin);
    ui::PushInset(MaxInt(50*rows, 50*tabs));
    ui::Shrink(5, 5);

    RID hovered_id = GetInvalidId();

    // Draw Selection Tabs
    int panel_width = ui::Current()->width;
    ui::PushHSplit(0, 50);
    int tabs_x = ui::Current()->x + ui::Current()->width;
    int tabs_y = ui::Current()->y;

    for(int i=0; i < tabs; i++) {
        Color tab_draw_color = (module_class_ui_tab == i) ? Palette::ui_main : Palette::ui_alt;
        ui::PushInset(48);
        ui::Shrink(4, 4);
        ui::EncloseEx(0, Palette::bg, tab_draw_color, 0);
        if (i < module_types::ANY) {
            ui::DrawIcon(module_types::icons[i], tab_draw_color, 40);
        } else {  // Ship classes
            // Hardcoded to draw transport ship icon
            ui::DrawIcon(AtlasPos(2, 28), tab_draw_color, 40);
        }
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

    // Draw options

    int draw_index = 0;
    for(int i=0; i < options[module_class_ui_tab].size; i++) {
        RID id = options[module_class_ui_tab].Get(i);
        AtlasPos atlas_pos;
        if (IsIdValidTyped(id, EntityType::SHIP_CLASS)) {
            atlas_pos = GetShipClassByRID(id)->icon_index;
        } else {
            atlas_pos = GetModule(id)->icon_index;
        }

        ui::PushGridCell(columns, rows, draw_index % columns, draw_index / columns);
        ui::Shrink(margin, margin);
        
        // Possible since Shipclasses get loaded once in continuous memory
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 4);
            hovered_id = id;
        } else {
            ui::Enclose();
        }
        HandleButtonSound(button_state);
        if ((button_state & button_state_flags::JUST_PRESSED)) {
            ship->production_queue.Append(id);
        }

        bool constructable = ship->CanProduce(id, false, true);
        if (constructable) {
            ui::DrawIcon(atlas_pos, Palette::ui_main, ui::Current()->height);
        } else {
            ui::DrawIcon(atlas_pos, Palette::ui_alt, ui::Current()->height);
        }
        //ui::FillLine(1.0, Palette::ui_main, Palette::bg);
        //ui::Write(ship_class->description);
        ui::Pop();  // GridCell
        draw_index++;
    }

    ui::Pop();  // HSplit from tabs
    ui::Pop();  // Inset

    // Draw queue

    bool hover_over_queue = false;
    for(int i=0; i < ship->production_queue.size; i++) {
        RID id = ship->production_queue.Get(i);

        AtlasPos atlas_pos;
        int total_construction_time;
        if (IsIdValidTyped(id, EntityType::SHIP_CLASS)) {
            total_construction_time = GetShipClassByRID(id)->construction_time;
            const ShipClass* sc = GetShipClassByRID(id);
            atlas_pos = sc->icon_index;
        } else if (IsIdValidTyped(id, EntityType::MODULE_CLASS)) {
            total_construction_time = GetShipModules()->GetModuleByRID(id)->construction_time;
            const ShipModuleClass* smc = GetModule(id);
            atlas_pos = smc->icon_index;
        }

        ui::PushInset(SHIP_MODULE_HEIGHT + margin + 2);
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
            if (i == 0) ship->production_process = 0;
            ship->production_queue.EraseAt(i);
            i--;
        }
        ui::DrawIcon(atlas_pos, Palette::ui_main, ui::Current()->height);
        double progress = ship->production_process / (double) total_construction_time;
        if (i == 0) {
            ui::FillLineEx(
                ui::Current()->x,
                ui::Current()->x + ui::Current()->width,
                ui::Current()->y + ui::Current()->height,
                progress, Palette::ui_main, Palette::bg);
        }
        ui::Pop();  // Inset
    }
    if (IsIdValid(hovered_id)) {
        ui::PushMouseHint(GetMousePosition(), 400, 400, 255 - MAX_TOOLTIP_RECURSIONS);
        ui::Current()->text_background = Palette::bg;
        ui::Current()->flexible = true;
        Planet* planet = GetPlanet(ship->GetParentPlanet());
        _ProductionQueueMouseHint(hovered_id, ship, hover_over_queue);
        ui::EncloseDynamic(5, Palette::bg, Palette::ui_main, 4);
        ui::Pop();
    }
 }

void Ship::DrawUI() {

    //          PANEL UI
    // =============================

    if (mouse_hover) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, Palette::ui_main);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }

    // Reset
    current_slot = ShipModuleSlot();
    highlighted_plan_index = -1;

    if (
        !mouse_hover 
        && GetTransferPlanUI()->ship != id
        && (GetGlobalState()->focused_ship != id || IsIdValid(GetGlobalState()->hover))
    ) return;


    if ((GetIntelLevel() & intel_level::STATS) == 0) {
        return;
    }

    const int OUTSET_MARGIN = 6;
    const int TEXT_SIZE = DEFAULT_FONT_SIZE;
    int width = (SHIP_MODULE_WIDTH + 3) * 6 + 9 + 8 + 3;
    int panel_start_x = GetScreenWidth() - width - OUTSET_MARGIN;
    int panel_start_y = OUTSET_MARGIN + 50;
    ui::CreateNew(
        panel_start_x, panel_start_y, width,  GetScreenHeight(),
        TEXT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE
    );

    //ui::EncloseEx(4, Palette::bg, GetColor(), 4);
    //ui::PushScrollInset(0, ui::Current()->height, allocated, &ui_scroll);

    _UIDrawHeader(this);

    ui::PushInset(-1);
        ui::VSpace(6);
        _UIDrawStats(this);
        ui::FillLine(1, Palette::ui_alt, Palette::bg);
        _UIDrawFleet(this);
        ui::VSpace(6);
        ui::EncloseDynamic(0, Palette::bg, Palette::ui_main, 4);
    ui::Pop();

    // Submenu toggles

    const int gap = 5;
    static const char* labels[] = {"Mods.", "Prod.", "Transf."};

    bool can_produce = IsParked() && CanProduce();

    ui::VSpace(gap);
    ui::PushInset(50);
    for (int i=0; i < 3; i++) {
        if (IsStatic() && i == 2) continue;
        if (!can_produce && i == 1) continue;
        ui::PushHSplit(50*i + gap*(i+1), 50*(i+1) + gap*(i+1));
            ui::Current()->text_color = ui_submenu_toggles[i] ? Palette::ui_main : Palette::ui_alt;
            ui::Enclose();
            ui::WriteEx(labels[i], text_alignment::CENTER, false);
            button_state_flags::T button_state = ui::AsButton();
            HandleButtonSound(button_state);
            if (button_state & button_state_flags::JUST_PRESSED) {
                ui_submenu_toggles[i] = !ui_submenu_toggles[i];
            }
        ui::Pop();
    }
    ui::Pop();
    ui::HelperText(GetUI()->GetConceptDescription("ship"));
    int lower_bound = ui::Current()->GetTextCursor().y;
    ui::Pop();
    if (ui_submenu_toggles[0]) {  // Modules
        GetShipClassByRID(ship_class)->module_config.Draw(this,
            {(float)panel_start_x - 10.0f, (float)panel_start_y}, 
            text_alignment::TOP | text_alignment::RIGHT);
    }
    if (ui_submenu_toggles[1]) {  // Production Queue
        ui::CreateNew(
            panel_start_x, lower_bound + 10, width, GetScreenHeight() - lower_bound - 10,
            TEXT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE
        );
        ui::Enclose();
        _UIDrawProduction(this);
    }
    if (ui_submenu_toggles[2]) {  // Transfers
        ui::CreateNew(
            panel_start_x, lower_bound + 10, width,  1,
            TEXT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE
        );
        ui::Current()->flexible = true;
        ui::Current()->z_layer += 5;
        int panel_height = _UIDrawTransferplans(this);
        ui::Pop();
        lower_bound += panel_height + 10;
        
        // Transfer panel
        int width = 20*DEFAULT_FONT_SIZE;
        ui::CreateNew(
            GetScreenWidth() - width - OUTSET_MARGIN, lower_bound + 5,
            width, 135, 
            DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE
        );
        GetGlobalState()->active_transfer_plan.DrawUI();
    }
    //if (!IsPlayerControlled()) {
    //    ui::Pop();  // ScrollInset
    //    return;
    //}
    //GetShipClassByRID(ship_class)->module_config.Draw(this);
    //if (!IsStatic()) {
    //    _UIDrawTransferplans(this);
    //    _UIDrawQuests(this);
    //}

    //ui::Pop();  // ScrollInset
}
