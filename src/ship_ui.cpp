#include "ship.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"

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

void _UIDrawStats(const Ship* ship) {
    const int combat_stat_block_height = 4 * 20;

    StringBuilder sb;
    sb.AddFormat(ICON_PAYLOAD " %d / %d ", KGToResourceCounts(ship->GetOperationalMass()), ship->GetMaxCapacity());
    ui::WriteEx(sb.c_str, text_alignment::LEFT | text_alignment::VCONFORM, false);
    sb.Clear();
    if (ship->IsStatic()) sb.AddFormat("\u0394V --       ", ship->GetCapableDV() / 1000.f);
    else sb.AddFormat("\u0394V %2.2f km/s  ", ship->GetCapableDV() / 1000.f);

    ui::WriteEx(sb.c_str, text_alignment::RIGHT | text_alignment::VCONFORM, false);
    ui::Fillline(ship->GetOperationalMass() / ResourceCountsToKG(ship->GetMaxCapacity()), Palette::ui_main, Palette::bg);
    ui::Write("");  // Linebreak
    ui::VSpace(15);
    ui::PushInset(combat_stat_block_height);
        sb.Clear();
        sb.AddFormat(ICON_POWER "%2d" ICON_ACS "%2d\n", ship->power(), ship->initiative());
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

void _UIDrawTransferplans(Ship* ship) {
    int inset_height = ship->prepared_plans_count * (ui::Current()->GetLineHeight() * 2 + 8 + 4);
    inset_height += 20;

    ui::PushInset(inset_height);
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
            departure_planet_name = GetPlanet(ship->prepared_plans[i].departure_planet)->name;
        } else {
            departure_planet_name = "NOT SET";
        }
        if (IsIdValid(ship->prepared_plans[i].arrival_planet)) {
            arrival_planet_name = GetPlanet(ship->prepared_plans[i].arrival_planet)->name;
        } else {
            arrival_planet_name = "NOT SET";
        }

        StringBuilder sb;
        sb.AddFormat(
            "- %s (%3d D %2d H)\n", resource_name,
            (int) (ship->prepared_plans[i].arrival_time - now).Seconds() / 86400,
            ((int) (ship->prepared_plans[i].arrival_time - now).Seconds() % 86400) / 3600
        );
        sb.AddFormat("  %s >> %s", departure_planet_name, arrival_planet_name);

        // Double Button
        ui::PushInset(ui::Current()->GetLineHeight() * 2);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EncloseEx(4, Palette::bg, Palette::interactable_main, 4);
        } else {
            ui::Enclose();
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
    ui::Pop();  // Inset
    
    // 'New transfer' button

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
        ui::Write(candidate->name);
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

    const int allocated = 1000;
    const int OUTSET_MARGIN = 6;
    const int TEXT_SIZE = DEFAULT_FONT_SIZE;
    int width = (SHIP_MODULE_WIDTH + 3) * 6 + 9 + 8 + 3;
    ui::CreateNew(
        GetScreenWidth() - width - OUTSET_MARGIN, 
        OUTSET_MARGIN + 200, width, 
        GetScreenHeight() - 200 - OUTSET_MARGIN,
        TEXT_SIZE, Palette::ui_main, Palette::bg, false
    );
    //ui::EncloseEx(4, Palette::bg, GetColor(), 4);

    //ui::PushScrollInset(0, ui::Current()->height, allocated, &ui_scroll);

    _UIDrawHeader(this);

    ui::PushInset(-1);
        ui::VSpace(6);
        _UIDrawStats(this);
        _UIDrawFleet(this);
        ui::VSpace(6);
        ui::EncloseDynamic(0, Palette::bg, Palette::ui_main, 4);
    ui::Pop();
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
    ui::HelperText(GetUI()->GetConceptDescription("ship"));
}
