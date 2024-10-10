#include "ship.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"
#include "utils.hpp"

void _UIDrawHeader(const Ship* ship) {
    ui::WriteEx(ship->name, text_alignment::CONFORM, false);    ui::WriteEx(ship->GetTypeIcon(), text_alignment::CONFORM, false);
    const char* text = ICON_EMPTY " " ICON_EMPTY "  ";  // use ICON_EMPTY instead of space to get the right spacing
    if (!ship->IsParked() && !ship->IsLeading()) text = ICON_EMPTY " " ICON_TRANSPORT_FLEET "  ";
    if (ship->IsParked() && ship->IsLeading()) text = ICON_PLANET " " ICON_EMPTY "  ";
    if (ship->IsParked() && !ship->IsLeading()) text = ICON_PLANET " " ICON_TRANSPORT_FLEET "  ";
    text::Layout layout;
    ui::Current()->GetTextLayout(&layout, text, text_alignment::RIGHT | text_alignment::VCONFORM);
    int pos = layout.GetCharacterIndex(GetMousePosition());
    ui::WriteEx(text, text_alignment::RIGHT | text_alignment::VCONFORM, true);
    button_state_flags::T planet_button_state = GetButtonState(pos >= 0 && pos < 2, false, ui::Current()->z_layer);  // Don't care about hover_in
    button_state_flags::T fleet_button_state = GetButtonState(pos >= 3 && pos < 5, false, ui::Current()->z_layer);  // Don't care about hover_in
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
        if (i > 0 && !global_vars::Get("combat_unlocked"))
            continue;
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
        else ui::WriteEx("Combat", text_alignment::VCENTER | text_alignment::RIGHT, false);
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
                text::Layout layout;
                ui::Current()->GetTextLayout(&layout, text, text_alignment::CONFORM);
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
        sb.Add("Strength >> ").AddI(ship->GetCombatStrength()).Add(" <<");

        ui::PushInset(25);
        ui::WriteEx(sb.c_str, text_alignment::CENTER, true);
        if (ui::AsButton() & button_state_flags::HOVER) {
            ui::SetMouseHint("Strength estimate: Summary of estimated\n"
                            "combat effectiveness if this ship");
        }
        ui::Pop();
        ui::VSpace(6);

        ui::PushInset(combat_stat_block_height);
        //sb.AddFormat(ICON_POWER "%2d" ICON_ACS "%2d\n", ship->power(), ship->initiative());
        sb.Clear();
        sb.AddFormat(" %2d/%2d" ICON_HEART_KINETIC /*"  %2d/%2d" ICON_HEART_ENERGY*/ "  %3d/%3d" ICON_HEART_BOARDING "\n", 
                    ship->kinetic_hp() - ship->damage_taken[ship_variables::HP], ship->kinetic_hp(),
                    //ship->energy_hp()  - ship->damage_taken[ship_variables::ENERGY_ARMOR],  ship->energy_hp(),
                    ship->crew()       - ship->damage_taken[ship_variables::CREW],          ship->crew());
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
    int panel_height = ui::Current()->GetLineHeight() * 2 + 8;
    int inset_height = ship->prepared_plans_count * panel_height;
    if (ship->transferplan_cycle.stops > 0) {
        inset_height += panel_height;
    }
    inset_height += 40;

    ui::PushInset(inset_height);
    ui::Enclose();

    if (!ship->IsLeading()) {
        ui::Write("Ship is not the leading ship in fleet. "
                  "Disconnect from fleet before assigning transfers");
        ui::Pop();  // Inset
        return 0;    }

    timemath::Time now = GlobalGetNow();
    for (int i=0; i < ship->prepared_plans_count; i++) {
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
        bool has_departure_planet = IsIdValidTyped(ship->prepared_plans[i].departure_planet, EntityType::PLANET);
        departure_planet_name = has_departure_planet ? GetPlanet(ship->prepared_plans[i].departure_planet)->name.GetChar() : "NOT SET";
        bool has_arrival_planet = IsIdValidTyped(ship->prepared_plans[i].arrival_planet, EntityType::PLANET);
        arrival_planet_name = has_arrival_planet ? GetPlanet(ship->prepared_plans[i].arrival_planet)->name.GetChar() : "NOT SET";

        StringBuilder sb;
        if (has_departure_planet && has_arrival_planet) {
            sb.AddFormat(
                "- %s (%3d D %2d H)\n", resource_name,
                (int) (ship->prepared_plans[i].arrival_time - now).Seconds() / timemath::SECONDS_IN_DAY,
                ((int) (ship->prepared_plans[i].arrival_time - now).Seconds() % timemath::SECONDS_IN_DAY) / 3600
            );
        }
        sb.AddFormat("  %s >> %s", departure_planet_name, arrival_planet_name);

        // Double Button
        ui::PushInset(panel_height);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EnclosePartial(4, Palette::bg, Palette::interactable_main, direction::DOWN | direction::LEFT);
        } else {
            ui::EnclosePartial(4, Palette::bg, Palette::ui_main, direction::DOWN | direction::LEFT);
        }
        ui::PushHSplit(0, -32);
        ui::Write(sb.c_str);
        if (button_state & button_state_flags::HOVER) {
            ship->highlighted_plan_index = i;
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            if (ship->transferplan_cycle.stops == 0) {
                HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
                ship->StartEditingPlan(i);
            } else {
                PlaySFX(SFX_ERROR);
            }
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
                if (ship->transferplan_cycle.stops == 0) {
                    HandleButtonSound(button_state & button_state_flags::JUST_PRESSED);
                    ship->RemoveTransferPlan(i);
                } else {
                    PlaySFX(SFX_ERROR);
                }
            }
            ui::Pop();  // HSplit
        }

        ui::Pop();  // Insert
    }
    
    ui::HelperText(GetUI()->GetConceptDescription("transfer"));

    if (ship->transferplan_cycle.stops > 0) {
        ui::PushInset(panel_height);
        StringBuilder sb;
        sb.Add("Currently on cycle: \n");
        for (int i=0; i < ship->transferplan_cycle.stops; i++) {
            sb.AddPerma(GetPlanet(ship->transferplan_cycle.planets[i])->name);
            sb.Add(" >> ");
        }
        sb.AddPerma(GetPlanet(ship->transferplan_cycle.planets[0])->name);
        ui::Write(sb.c_str);

        ui::Pop();  // Inset
    }
    
    // Buttons

    ui::VSpace(10);

    ui::PushInset(30);
    RID last_arrival_planet = ship->prepared_plans[ship->prepared_plans_count - 1].arrival_planet;
    bool show_cycle_button = 
        ship->transferplan_cycle.stops > 0 || 
        (ship->GetParentPlanet() == last_arrival_planet && ship->plan_edit_index < 0);
    show_cycle_button = show_cycle_button && global_vars::TryGetVar(HashKey("cycles_unlocked"));
    bool show_add_button = ship->transferplan_cycle.stops == 0;
    int button_tabs = 2;
    bool button_pressed[2];
    for(int i=0; i < button_tabs; i++) {
        ui::PushHSplit(i * ui::Current()->width/button_tabs,
                       (i+1) * ui::Current()->width/button_tabs);

        if ((i == 0 && !show_add_button) || (i == 1 && !show_cycle_button)) {
            ui::Pop();  // HSplit
            continue;
        }

        const char* text = "New Transfer";
        if (i == 1) {  // Cycles
            text = ship->transferplan_cycle.stops > 0 ? "Remove Cycle" : "Add Cycle";
        }

        button_state_flags::T button_state = ui::WriteButton(text);
        button_pressed[i] = button_state & button_state_flags::JUST_PRESSED;
        if (i == 1 && ship->transferplan_cycle.stops == 0 && button_state & button_state_flags::HOVER) {
            StringBuilder sb;
            for (int i=0; i < ship->prepared_plans_count; i++) {
                sb.AddPerma(GetPlanet(ship->prepared_plans[i].departure_planet)->name);
                sb.Add(" >> ");
            }
            ui::SetMouseHint(sb.c_str);
        }

        ui::Pop();  // HSplit
    }
    
    ui::Pop();  // Inset (Button)

    if (button_pressed[0]) {
        ship->_OnNewPlanClicked();
    }
    if (button_pressed[1]) {
        if (ship->transferplan_cycle.stops > 0) {
            ship->transferplan_cycle.Reset();
        } else {
            ship->transferplan_cycle.GenFromTransferplans(ship->prepared_plans, ship->prepared_plans_count);
        }
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

void _ProductionQueueMouseHint(RID id, const Ship* ship, bool is_in_production_queue) {
    if (!IsIdValid(id)) return;
    // Assuming monospace font
    
    const Planet* planet = GetPlanet(ship->GetParentPlanet());

    int char_width = ui::Current()->GetCharWidth();
    const resource_count_t* construction_resources = NULL;
    const int* construction_requirements = NULL;
    int build_time = 0;
    int batch_size = 0;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByRID(id);
            ship_class->MouseHintWrite();
            construction_resources = &ship_class->construction_resources[0];
            construction_requirements =  &ship_class->construction_requirements[0];
            build_time = ship_class->construction_time;
            batch_size = ship_class->construction_batch_size;
            break;
        }
        case EntityType::MODULE_CLASS: {
            const ShipModuleClass* module_class = GetModule(id);
            module_class->MouseHintWrite();
            construction_resources = &module_class->construction_resources[0];
            construction_requirements =  &module_class->construction_requirements[0];
            build_time = module_class->GetConstructionTime();
            batch_size = module_class->construction_batch_size;
            break;
        }
        default: break;
    }
    
    StringBuilder sb;

    ui::VSpace(6);
    ui::FillLine(1, Palette::ui_main, Palette::ui_main);
    ui::Current()->LineBreak();

    if (construction_resources == NULL) return;
    if (construction_requirements == NULL) return;

    // Build requirements

    ui::VSpace(6);
    ui::WriteEx("To build:", text_alignment::HCENTER | text_alignment::VCONFORM, true);

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

    // Notify if planet is invalid
    
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

    if (IsIdValidTyped(id, EntityType::MODULE_CLASS)) {
        const ShipModuleClass* module_class = GetModule(id);
        bool is_planet_invalid = module_class->IsPlanetRestricted(planet->id);
        if (is_planet_invalid) {
            sb.Clear();
            sb.Add("Not working on this planet - Use instead on:\n");
            for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
                if ((module_class->planets_restriction >> i) & 1) {
                    sb.AddPerma(GetPlanetByIndex(i)->name).Add(", ");
                }
            }
            ui::VSpace(6);
            ui::PushInset(70);
            ui::Current()->x += 5;
            ui::Current()->text_color = Palette::red;
            ui::EncloseEx(0, Palette::bg, Palette::red, 0);
            ui::Write(sb.c_str);
            //ui::FillLine(1, Palette::red, Palette::red);
            ui::Current()->text_color = Palette::ui_main;
            ui::Pop();  // Inset
        }
    }
        
    ui::VSpace(8);
    ui::FillLine(1, Palette::ui_alt, Palette::ui_alt);
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

void _UIDrawProduction(Ship* ship) {
    static int module_class_ui_tab = 0;
    const int tabs = module_types::ANY + 1;

    if (ship == NULL) {
        return;
    }
    if (!ship->IsParked()) {
        return;
    }

    // Set variables for planet/ship
    int option_size = GetShips()->ship_classes_count + GetShipModules()->shipmodule_count;
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
        if (options[i].Count() > max_display_options) {
            max_display_options = options[i].Count();
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
        if (options[i].Count() == 0) {
            continue;
        }
        Color tab_draw_color = (module_class_ui_tab == i) ? Palette::ui_main : Palette::ui_alt;
        ui::PushInset(48);
        ui::Shrink(4, 4);
        ui::EncloseEx(0, Palette::bg, tab_draw_color, 0);
        if (i < module_types::ANY) {
            ui::DrawIcon(module_types::icons[i], text_alignment::CENTER, tab_draw_color, 40);
        } else {  // Ship classes
            // Hardcoded to draw transport ship icon
            ui::DrawIcon(AtlasPos(2, 28), text_alignment::CENTER, tab_draw_color, 40);
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
            ui::DrawIcon(atlas_pos, text_alignment::CENTER, Palette::ui_main, 40);
        } else {
            ui::DrawIcon(atlas_pos, text_alignment::CENTER, Palette::ui_alt, 40);
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
        ui::DrawIcon(atlas_pos, text_alignment::CENTER, Palette::ui_main, 40);
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
        Vector2 pos = { GetScreenWidth() - 420, GetMousePosition().y + 10 };
        ui::PushMouseHint(pos, 400, 400);
        ui::Current()->text_background = Palette::bg;
        ui::Current()->flexible = true;
        Planet* planet = GetPlanet(ship->GetParentPlanet());
        _ProductionQueueMouseHint(hovered_id, ship, hover_over_queue);
        ui::Current()->extend_x = 400;  // force x-size
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
        && (GetGlobalState()->focused_ship != id || IsIdValidTyped(GetGlobalState()->hover, EntityType::SHIP))
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
    static const char* labels[] = {"Mods.", "Prod.", "Transf."};  // TODO: can be icons

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
}
