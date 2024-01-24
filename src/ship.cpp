#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"
#include "combat.hpp"

double ShipClass::GetPayloadCapacityMass(double dv) const {
    //          dv = v_e * ln((max_cap + eom) / (x + eom))
    // <=> x + eom = (max_cap + eom) / exp(dv/v_e)

    return (ResourceCountsToKG(max_capacity) + oem) / exp(dv/v_e) - oem;
}

resource_count_t ShipClass::GetFuelRequiredFull(double dv) const {
    // assuming max payload
    return max_capacity - KGToResourceCounts(GetPayloadCapacityMass(dv));
}

resource_count_t ShipClass::GetFuelRequiredEmpty(double dv) const {
    // assuming no payload
    return KGToResourceCounts(oem * (exp(dv/v_e) - 1));
}

Ship::Ship() {
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }
}

Ship::~Ship() {
    
}

bool Ship::HasMouseHover(double* min_distance) const {
    double dist = Vector2Distance(GetMousePosition(), draw_pos);
    if (dist <= 10 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}

void Ship::_OnClicked() {
    if (GlobalGetState()->focused_ship == id) {
        GetScreenTransform()->focus = position.cartesian;
    } else {
        GlobalGetState()->focused_ship = id;
    }
    HandleButtonSound(ButtonStateFlags::JUST_PRESSED);
}

void Ship::_OnNewPlanClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (tp_ui.IsActive() || IsIdValid(tp_ui.ship)) {
        return;
    }

    GetCalendar()->paused = true;

    // Append new plan

    ASSERT(prepared_plans_count != 0 || IsParked())
    if (prepared_plans_count >= SHIP_MAX_PREPARED_PLANS) {
        ERROR("Maximum transfer plan stack reached (ship %s)", name);
        return;
    }

    prepared_plans[prepared_plans_count] = TransferPlan();
    plan_edit_index = prepared_plans_count;

    timemath::Time min_time = 0;
    if (plan_edit_index == 0) {
        prepared_plans[plan_edit_index].departure_planet = parent_obj;
        min_time = GlobalGetNow();
    } else {
        prepared_plans[plan_edit_index].departure_planet = prepared_plans[plan_edit_index - 1].arrival_planet;
        min_time = prepared_plans[plan_edit_index - 1].arrival_time;
    }

    tp_ui.Reset();
    tp_ui.SetPlan(&prepared_plans[plan_edit_index], id, min_time, 1e20);
    prepared_plans_count++;
}

void Ship::Serialize(DataNode *data) const
{
    data->Set("name", name);
    data->SetI("allegiance", allegiance);
    
    data->Set("class_id", GetShipClassByIndex(ship_class)->id);
    data->SetI("resource_qtt", transporing.quantity);
    data->SetI("resource_id", transporing.resource_id);
    data->SetArray("dammage_taken", ShipVariables::MAX);
    for(int i=0; i < ShipVariables::MAX; i++) {
        data->SetArrayElemI("dammage_taken", i, dammage_taken[i]);
    }

    data->SetArray("modules", SHIP_MAX_MODULES);
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) {
            data->SetArrayElem("modules", i, GetModule(modules[i])->id);
        } else {
            data->SetArrayElem("modules", i, "---");
        }
    }

    data->SetArrayChild("prepared_plans", prepared_plans_count);
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i].Serialize(data->SetArrayElemChild("prepared_plans", i, DataNode()));
    }
}

void Ship::Deserialize(const DataNode* data) {
    strcpy(name, data->Get("name", "UNNAMED"));
    allegiance = data->GetI("allegiance", allegiance);
    plan_edit_index = -1;
    ship_class = GlobalGetState()->ships.GetShipClassIndexById(data->Get("class_id"));
    if (!IsIdValid(ship_class)) {
        FAIL("Invalid ship class")  // TODO fail more gracefully
    }
    transporing.quantity = data->GetI("resource_qtt", 0, true);
    transporing.resource_id = (ResourceType) data->GetI("resource_id", RESOURCE_NONE, true);

    int modules_count = data->GetArrayLen("modules", true);
    if (modules_count > SHIP_MAX_MODULES) modules_count = SHIP_MAX_MODULES;
    const ShipModules* sms = &GlobalGetState()->ship_modules;
    for(int i=0; i < modules_count; i++) {
        modules[i] = sms->GetModuleRIDFromStringId(data->GetArray("modules", i));
    }
    for(int i=modules_count; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }

    prepared_plans_count = data->GetArrayChildLen("prepared_plans", true);
    if (prepared_plans_count > SHIP_MAX_PREPARED_PLANS) prepared_plans_count = SHIP_MAX_PREPARED_PLANS;
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i] = TransferPlan();
        prepared_plans[i].Deserialize(data->GetArrayChild("prepared_plans", i, true));
    }

    dammage_taken[ShipVariables::KINETIC_ARMOR] = data->GetArrayI("dammage_taken", ShipVariables::KINETIC_ARMOR, stats[ShipStats::KINETIC_HP], true);
    dammage_taken[ShipVariables::ENERGY_ARMOR] = data->GetArrayI("dammage_taken", ShipVariables::ENERGY_ARMOR, stats[ShipStats::ENERGY_HP], true);
    dammage_taken[ShipVariables::CREW] = data->GetArrayI("dammage_taken", ShipVariables::CREW, stats[ShipStats::CREW], true);
}

double Ship::GetPayloadMass() const{
    double res = 0.0;

    QuestManager* qm = &GlobalGetState()->quest_manager;
    for(auto i = qm->active_tasks.GetIter(); i; i++) {
        if (qm->active_tasks[i]->ship == id) {
            res += qm->active_tasks[i]->payload_mass;
        }
    }

    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) 
            res += GetModule(modules[i])->mass;
    }

    return res - 0;
}

resource_count_t Ship::GetMaxCapacity() const {
    return GetShipClassByIndex(ship_class)->max_capacity;
}

resource_count_t Ship::GetRemainingPayloadCapacity(double dv) const {
    const ShipClass* sc = GetShipClassByIndex(ship_class);
    double capacity = sc->GetPayloadCapacityMass(dv);
    return KGToResourceCounts(capacity - GetPayloadMass());
}

resource_count_t Ship::GetFuelRequiredEmpty(double dv) const {
    const ShipClass* sc = GetShipClassByIndex(ship_class);
    return KGToResourceCounts((sc->oem + GetPayloadMass()) * (exp(dv/sc->v_e) - 1));
}

double Ship::GetCapableDV() const {
    const ShipClass* sc = GetShipClassByIndex(ship_class);
    return sc->v_e * log((ResourceCountsToKG(sc->max_capacity) + sc->oem) / (GetPayloadMass() + sc->oem));

    // max_dv = v_e * log(max_capacity / oem + 1)
    // exp(max_dv / v_e) - 1

    // oem = max_capacity / (exp(max_dv/v_e) - 1);
    // sc->v_e * log(max_capacity / oem + 1);
}

bool Ship::IsPlayerFriend() const {
    return allegiance == 0;
}

IntelLevel::Type Ship::GetIntelLevel() const {
    return IntelLevel::FULL;

    if (IsPlayerFriend()) {
        return IntelLevel::FULL;
    }
    return IntelLevel::STATS | (is_detected * IntelLevel::TRAJECTORY);
}

bool Ship::IsTrajectoryKnown(int index) const {
    if (IsPlayerFriend()) {
        return true;
    }
    if((GetIntelLevel() & IntelLevel::TRAJECTORY) == 0) {
        return false;
    }
    return index == 0 && !IsParked();
}

int Ship::GetCombatStrength() const {
    int offensive = kinetic_offense() + ordnance_offense();
    int defensive = kinetic_defense() + ordnance_defense();
    return offensive * (defensive + 1);
}

int Ship::GetMissingHealth() const {
    //SHOW_I(stats[ShipStats::KINETIC_HP])
    //SHOW_I(stats[ShipStats::ENERGY_HP])
    //SHOW_I(stats[ShipStats::CREW])
    //SHOW_I(variables[ShipVariables::KINETIC_ARMOR])
    //SHOW_I(variables[ShipVariables::ENERGY_ARMOR])
    //SHOW_I(variables[ShipVariables::CREW])
    
    return
          dammage_taken[ShipVariables::KINETIC_ARMOR]
        + dammage_taken[ShipVariables::ENERGY_ARMOR]
        + dammage_taken[ShipVariables::CREW]
    ;
}

bool Ship::CanDragModule(int index) const {
    if (!IsIdValid(modules[index]))
        return true;
    if (GetModule(modules[index])->delta_stats[ShipStats::KINETIC_HP] > kinetic_hp() - dammage_taken[ShipVariables::KINETIC_ARMOR]) {
        return false;
    }
    if (GetModule(modules[index])->delta_stats[ShipStats::ENERGY_HP] > energy_hp() - dammage_taken[ShipVariables::ENERGY_ARMOR]) {
        return false;
    }
    if (GetModule(modules[index])->delta_stats[ShipStats::CREW] > crew() - dammage_taken[ShipVariables::CREW]) {
        return false;
    }
    return true;
}

Color Ship::GetColor() const {
    return IsPlayerFriend() ? Palette::green : Palette::red;
}

bool Ship::IsParked() const {
    switch (IdGetType(parent_obj)) {
    case EntityType::PLANET: return true;
    case EntityType::INVALID: return false;
    case EntityType::SHIP: return GetShip(parent_obj)->IsParked();
    default: NOT_REACHABLE
    }
}

bool Ship::IsLeading() const {
    return IdGetType(parent_obj) != EntityType::SHIP;
}

RID Ship::GetParentPlanet() const {
    switch (IdGetType(parent_obj)) {
    case EntityType::PLANET: return parent_obj;
    case EntityType::INVALID: return GetInvalidId();
    case EntityType::SHIP: return GetShip(parent_obj)->GetParentPlanet();
    default: NOT_REACHABLE
    }
}

TransferPlan* Ship::GetEditedTransferPlan() {
    if (plan_edit_index < 0) return NULL;
    return &prepared_plans[plan_edit_index];
}

void Ship::ConfirmEditedTransferPlan() {
    ASSERT(prepared_plans_count != 0 || IsParked())
    ASSERT(!IsParked() || IsIdValid(parent_obj))
    TransferPlan* tp = &prepared_plans[prepared_plans_count - 1];
    if (prepared_plans_count == 1 && parent_obj != tp->departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at current planet)", name);
        return;
    }
    else if (prepared_plans_count > 1 && prepared_plans[prepared_plans_count - 2].arrival_planet != tp->departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at planet last visited)", name);
        return;
    }
    double dv_tot = tp->dv1[tp->primary_solution] + tp->dv2[tp->primary_solution];
    if (dv_tot > GetShipClassByIndex(ship_class)->max_dv) {
        ERROR("Not enough DV %f > %f", dv_tot, GetShipClassByIndex(ship_class));
        return;
    }
    //payload.push_back(TransportContainer(tp.resource_transfer));
    INFO("Assigning transfer plan to ship %s", name);
    plan_edit_index = -1;
}

void Ship::CloseEditedTransferPlan() {
    int index = plan_edit_index;
    plan_edit_index = -1;
    RemoveTransferPlan(index);
}

void Ship::RemoveTransferPlan(int index) {
    // Does not call _EnsureContinuity to prevent invinite recursion
    if (index < 0 || index >= prepared_plans_count) {
        ERROR("Tried to remove transfer plan at invalid index %d (ship %s)", index, name);
        return;
    }

    if (index == plan_edit_index) {
        ERROR("Stop editing plan before attempting to remove it");
        return;
    }

    for (int i=index; i < prepared_plans_count - 1; i++) {
        prepared_plans[i] = prepared_plans[i+1];
    }
    prepared_plans_count--;
    if (index == plan_edit_index) {
        plan_edit_index = -1;
    }
}

void Ship::StartEditingPlan(int index) {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    timemath::Time min_time = 0;
    if (index == 0) {
        if (IsParked()) {
            prepared_plans[index].departure_planet = parent_obj;
            min_time = GlobalGetNow();
        } else {
            return;
        }
    } else {
        prepared_plans[index].departure_planet = prepared_plans[index - 1].arrival_planet;
        min_time = prepared_plans[index - 1].arrival_time;
    }

    plan_edit_index = index;
    tp_ui.SetPlan(&prepared_plans[index], id, min_time, 1e20);
}

void Ship::Update() {
    timemath::Time now = GlobalGetNow();

    if (!IsLeading()) {
        // Following other ship
        if (IsParked()) {
            position = GetPlanet(GetParentPlanet())->position;
        } else {
            const TransferPlan* tp = &GetShip(parent_obj)->prepared_plans[0];
            position = tp->transfer_orbit[tp->primary_solution].GetPosition(now);
        }
    } else if (prepared_plans_count == 0 || (plan_edit_index == 0 && prepared_plans_count == 1)) {
        // No departure plans to draw from
        position = GetPlanet(parent_obj)->position;
    } else {
        const TransferPlan* tp = &prepared_plans[0];
        if (IsParked()) {
            if (tp->departure_time < now) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(parent_obj)->position;
            }
        } else {
            if (tp->arrival_time < now) {
                _OnArrival(tp);
            } else {
                position = tp->transfer_orbit[tp->primary_solution].GetPosition(now);
            }
        }
    }

    // Draw
    draw_pos = GetScreenTransform()->TransformV(position.cartesian);
    if (IsParked()) {
        double rad = fmax(GetScreenTransform()->TransformS(GetPlanet(parent_obj)->radius), 4) + 8.0;
        double phase = fmin(20.0 / rad * index_on_planet, index_on_planet / (double)total_on_planet * 2 * PI);
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
    }

    _UpdateModules();
    _UpdateShipyard();
}

void Ship::_UpdateShipyard() {
    int collected_components[4] = {0};
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        if(modules[i] == GlobalGetState()->ship_modules.expected_modules.small_yard_1) collected_components[0]++;
        if(modules[i] == GlobalGetState()->ship_modules.expected_modules.small_yard_2) collected_components[1]++;
        if(modules[i] == GlobalGetState()->ship_modules.expected_modules.small_yard_3) collected_components[2]++;
        if(modules[i] == GlobalGetState()->ship_modules.expected_modules.small_yard_4) collected_components[3]++;
    }
    int module_factories = MinInt(collected_components[1], collected_components[2]);
    int repair_stations = MinInt(collected_components[2], collected_components[3]);
    int ship_yards = MinInt(
        MinInt(collected_components[0], collected_components[1]),
        repair_stations
    );
    if (GlobalGetState()->calendar.IsNewDay() && IsParked()) {
        for(int i=0; i < repair_stations; i++) {
            IDList available_ships;
            uint32_t filter = ((1 << allegiance) & 0xFFU) | 0xFFFFFF00U;
            GlobalGetState()->ships.GetOnPlanet(&available_ships, parent_obj, filter);
            available_ships.Sort([](RID ship1, RID ship2){
                return GetShip(ship2)->GetMissingHealth() - GetShip(ship1)->GetMissingHealth();
            });
            int hp_pool = 5;
            for(int i=0; i < available_ships.size; i++) {
                // Knowingly include self
                int hp_needed = GetShip(available_ships[i])->GetMissingHealth();
                int hp_provided = MinInt(hp_pool, hp_needed);
                GetShip(available_ships[i])->Repair(hp_provided);
                hp_pool -= hp_provided;
                if (hp_pool <= 0) break;
            }
        }
        for(int i=0; i < module_factories; i++) {
            GetPlanet(parent_obj)->AdvanceModuleProductionQueue();
        }
        for(int i=0; i < ship_yards; i++) {
            GetPlanet(parent_obj)->AdvanceShipProductionQueue();
        }
    }
}

void Ship::_UpdateModules() {
    // Recalculate every frame

    // Gotta exist a more efficient way
    // And robust. Dependency graph (also a way to find circular dependencies)

    for (int i=0; i < ShipStats::MAX; i++) {
        stats[i] = GetShipClassByIndex(ship_class)->stats[i];
    }
    for (int j=0; j < SHIP_MAX_MODULES; j++) {
        if (IsIdValid(modules[j]) && !GetModule(modules[j])->HasDependencies()) {
            GetModule(modules[j])->UpdateStats(this);
        }
    }
    for (int j=0; j < SHIP_MAX_MODULES; j++) {
        if (IsIdValid(modules[j]) && GetModule(modules[j])->HasDependencies()) {
            GetModule(modules[j])->UpdateStats(this);
        }
    }
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) {
            GetModule(modules[i])->UpdateCustom(this);
        }
    }
}

void _DrawShipAt(Vector2 pos, Color color) {
    DrawRectangleV(Vector2SubtractValue(pos, 4.0f), {8, 8}, color);
}

void Ship::Draw(const CoordinateTransform* c_transf) const {
    if ((GetIntelLevel() & IntelLevel::TRAJECTORY) == 0) {
        return;
    }
    Color color = GetColor();
    _DrawShipAt(draw_pos, color);

    for (int i=0; i < prepared_plans_count; i++) {
        if (!IsTrajectoryKnown(i)) {
            continue;
        }
        if (i == plan_edit_index) {
            continue;
        }
        const TransferPlan* plan = &prepared_plans[i];
        OrbitPos to_departure = plan->transfer_orbit[plan->primary_solution].GetPosition(
            timemath::Time::Latest(plan->departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = plan->transfer_orbit[plan->primary_solution].GetPosition( 
            plan->arrival_time
        );
        plan->transfer_orbit[plan->primary_solution].DrawBounded(to_departure, to_arrival, 0, 
            ColorAlpha(color, i == highlighted_plan_index ? 1 : 0.5)
        );
        if (i == plan_edit_index){
            plan->transfer_orbit[plan->primary_solution].DrawBounded(to_departure, to_arrival, 0, Palette::ui_main);
        }
    }
    if (plan_edit_index >= 0 && IsIdValid(prepared_plans[plan_edit_index].arrival_planet) && IsTrajectoryKnown(plan_edit_index)) {
        const TransferPlan* last_tp = &prepared_plans[plan_edit_index];
        OrbitPos last_pos = GetPlanet(last_tp->arrival_planet)->orbit.GetPosition(
            last_tp->arrival_time
        );
        Vector2 last_draw_pos = c_transf->TransformV(last_pos.cartesian);
        _DrawShipAt(last_draw_pos, ColorAlpha(color, 0.5));
    }
}

void _UIDrawStats(const Ship* ship) {
    StringBuilder sb;
    sb.AddFormat("Payload %d / %d ", KGToResourceCounts(ship->GetPayloadMass()), ship->GetMaxCapacity());
    UIContextWrite(sb.c_str);
    UIContextFillline(ship->GetPayloadMass() / ResourceCountsToKG(ship->GetMaxCapacity()), Palette::ui_main, Palette::bg);
    sb.Clear();
    sb.AddFormat("dv %2.2f (I_sp: %2.2f)\n", ship->GetCapableDV(), GetShipClassByIndex(ship->ship_class)->v_e / 1000);
    sb.AddFormat("Pw. %2d Init. %2d\n", ship->power(), ship->initiative());
    sb.AddFormat("HP: %2d/%2d K - %2d/%2d E\n", 
        ship->kinetic_hp() - ship->dammage_taken[ShipVariables::KINETIC_ARMOR], ship->kinetic_hp(),
        ship->energy_hp() - ship->dammage_taken[ShipVariables::ENERGY_ARMOR], ship->energy_hp()
    );
    sb.AddFormat("Crew: %2d/%2d\n", ship->crew() - ship->dammage_taken[ShipVariables::CREW], ship->crew());
    sb.AddFormat("Offense: %2d K - %2d O - %2d B\n", ship->kinetic_offense(), ship->ordnance_offense(), ship->boarding_offense());
    sb.AddFormat("Defense: %2d K - %2d O - %2d B\n", ship->kinetic_defense(), ship->ordnance_defense(), ship->boarding_defense());
    sb.AddFormat("++++++++++++++++++\n");
    UIContextWrite(sb.c_str);
}

int ship_selecting_module_index = -1;
void _UIDrawInventory(Ship* ship) {
    const int MARGIN = 3;

    ShipModules* sms = &GlobalGetState()->ship_modules;
    int columns = UIContextCurrent().width / (SHIP_MODULE_WIDTH + MARGIN);
    int rows = std::ceil(SHIP_MAX_MODULES / (double)columns);

    UIContextPushInset(0, rows * (SHIP_MODULE_HEIGHT + MARGIN));
    UIContextCurrent().width = columns * (SHIP_MODULE_WIDTH + MARGIN);
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        UIContextPushGridCell(columns, rows, i % columns, i / columns);
        UIContextShrink(MARGIN, MARGIN);
        ButtonStateFlags::T button_state = UIContextAsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            ship->current_slot = ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            sms->InitDragging(ship->current_slot, UIContextCurrent().render_rec);
        }
        sms->DrawShipModule(ship->modules[i]);
        UIContextPop(); // GridCell
    }
    UIContextPop(); // Inset
}

void _UIDrawTransferplans(Ship* ship) {
    timemath::Time now = GlobalGetNow();
    for (int i=0; i < ship->prepared_plans_count; i++) {
        char tp_str[2][40];
        const char* resource_name;
        const char* departure_planet_name;
        const char* arrival_planet_name;

        if (ship->prepared_plans[i].resource_transfer.resource_id < 0){
            resource_name = "EMPTY";
        } else {
            resource_name = GetResourceData(ship->prepared_plans[i].resource_transfer.resource_id)->name;
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

        snprintf(tp_str[0], 40, "- %s (%3d D %2d H)",
            resource_name,
            (int) (ship->prepared_plans[i].arrival_time - now).Seconds() / 86400,
            ((int) (ship->prepared_plans[i].arrival_time - now).Seconds() % 86400) / 3600
        );

        snprintf(tp_str[1], 40, "  %s >> %s", departure_planet_name, arrival_planet_name);

        // Double Button
        UIContextPushInset(2, UIContextCurrent().GetLineHeight() * 2);
        UIContextPushHSplit(0, -32);
        UIContextEnclose(Palette::bg, Palette::blue);
        UIContextWrite(tp_str[0]);
        UIContextWrite(tp_str[1]);
        ButtonStateFlags::T button_state = UIContextAsButton();
        HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
        if (button_state & ButtonStateFlags::HOVER) {
            ship->highlighted_plan_index = i;
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            ship->StartEditingPlan(i);
        }
        UIContextPop();  // HSplit

        if (i != ship->plan_edit_index) {
            UIContextPushHSplit(-32, -1);
            UIContextWrite("X");
            UIContextEnclose(Palette::bg, Palette::blue);
            ButtonStateFlags::T button_state = UIContextAsButton();
            HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
            if (button_state & ButtonStateFlags::JUST_PRESSED) {
                ship->RemoveTransferPlan(i);
            }
            UIContextPop();  // HSplit
        }

        UIContextPop();  // Insert
    }
    if (UIContextDirectButton("+", 10) & ButtonStateFlags::JUST_PRESSED) {
        ship->_OnNewPlanClicked();
    }
}

void _UIDrawFleet(Ship* ship) {
    // Following
    if (!ship->IsLeading()) {
        UIContextPushInset(0, 20);
        StringBuilder sb;
        if (ship->IsParked()) {
            sb.Add("Detach from").Add(GetShip(ship->parent_obj)->name);
            ButtonStateFlags::T button_state = UIContextDirectButton(sb.c_str, 0);
            HandleButtonSound(button_state);
            if (button_state & ButtonStateFlags::JUST_PRESSED) {
                ship->Detach();
            }
        } else {
            sb.Add("Following").Add(GetShip(ship->parent_obj)->name);
            UIContextWrite(sb.c_str);
        }
        UIContextPop();  // Inset
        return;
    }
    IDList candidates;
    uint32_t selection_flags = ((1UL << ship->allegiance) & 0xFF) | 0xFFFFFF00;
    GlobalGetState()->ships.GetOnPlanet(&candidates, ship->GetParentPlanet(), selection_flags);
    for (int i=0; i < candidates.size; i++) {
        if (candidates[i] == ship->id) continue;
        const Ship* candidate = GetShip(candidates[i]);
        if (!candidate->IsLeading()) continue;
        UIContextPushInset(0, 20);
        UIContextWrite("Attach to ", false);
        UIContextWrite(candidate->name);
        ButtonStateFlags::T button_state = UIContextAsButton();
        HandleButtonSound(button_state);
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            ship->AttachTo(candidate->id);
        }
        UIContextPop();  // Inset
    }
}

void _UIDrawQuests(Ship* ship) {
    QuestManager* qm = &GlobalGetState()->quest_manager;
    double max_mass = ResourceCountsToKG(ship->GetMaxCapacity()) - ship->GetPayloadMass();
    
    for(auto it = qm->active_tasks.GetIter(); it; it++) {
        Task* quest = qm->active_tasks.Get(it);
        bool is_quest_in_cargo = quest->ship == ship->id;
        if (quest->current_planet != ship->parent_obj && !is_quest_in_cargo) continue;
        bool can_accept = quest->payload_mass <= max_mass;
        ButtonStateFlags::T button_state = quest->DrawUI(true, is_quest_in_cargo);
        if (button_state & ButtonStateFlags::JUST_PRESSED && can_accept) {
            if (is_quest_in_cargo) {
                qm->PutbackTask(ship->id, it.GetId());
            } else {
                qm->PickupTask(ship->id, it.GetId());
            }
        }
    }
}

void Ship::DrawUI() {
    if (mouse_hover) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, Palette::red);
        DrawTextEx(GetCustomDefaultFont(), name, Vector2Add(draw_pos, {5, 5}), 16, 1, GetColor());

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }

    // Reset
    current_slot = ShipModuleSlot();
    highlighted_plan_index = -1;

    if (!mouse_hover && GlobalGetState()->active_transfer_plan.ship != id && GlobalGetState()->focused_ship != id) return;

    const int INSET_MARGIN = 6;
    const int OUTSET_MARGIN = 6;
    const int TEXT_SIZE = 16;
    UIContextCreateNew(
        GetScreenWidth() - 430 - 2*INSET_MARGIN - OUTSET_MARGIN, 
        OUTSET_MARGIN + 200,
        430 + 2*INSET_MARGIN, 
        GetScreenHeight() - 200 - OUTSET_MARGIN - 2*INSET_MARGIN,
        TEXT_SIZE, Palette::ui_main
    );
    UIContextEnclose(Palette::bg, GetColor());

    UIContextShrink(INSET_MARGIN, INSET_MARGIN);

    if ((GetIntelLevel() & IntelLevel::STATS) == 0) {
        return;
    }

    const int allocated = 1000;

    UIContextPushScrollInset(0, UIContextCurrent().height, allocated, &ui_scroll);

    UIContextWrite(name);
    _UIDrawStats(this);
    if (!IsPlayerFriend()) {
        UIContextPop();  // ScrollInset
        return;
    }
    _UIDrawInventory(this);
    _UIDrawTransferplans(this);
    _UIDrawFleet(this);
    _UIDrawQuests(this);

    UIContextPop();  // ScrollInset
}

void Ship::Inspect() {

}

void Ship::RemoveShipModuleAt(int index) {
    modules[index] = GetInvalidId();
}

void Ship::Repair(int hp) {
    if (hp > stats[ShipStats::KINETIC_HP] + stats[ShipStats::ENERGY_HP] + stats[ShipStats::CREW]){
        // for initializetion
        dammage_taken[ShipVariables::KINETIC_ARMOR] = 0;
        dammage_taken[ShipVariables::ENERGY_ARMOR] = 0;
        dammage_taken[ShipVariables::CREW] = 0;
        return;
    }
    int kinetic_repair = MinInt(hp, dammage_taken[ShipVariables::KINETIC_ARMOR]);
    hp -= kinetic_repair;
    int energy_repair = MinInt(hp, dammage_taken[ShipVariables::ENERGY_ARMOR]);
    hp -= energy_repair;
    int crew_repair = MinInt(hp, dammage_taken[ShipVariables::CREW]);
    dammage_taken[ShipVariables::KINETIC_ARMOR] -= kinetic_repair;
    dammage_taken[ShipVariables::ENERGY_ARMOR] -= energy_repair;
    dammage_taken[ShipVariables::CREW] -= crew_repair;
}

void Ship::AttachTo(RID parent_ship) {
    if (!GetShip(parent_ship)->IsLeading()) {
        ERROR("Can only attach to leading ships")
        return;
    }
    parent_obj = parent_ship;
}

void Ship::Detach() {
    if (!IsParked()) {
        ERROR("Must be parked to detach")
        return;
    }
    parent_obj = GetParentPlanet();
}

void Ship::_OnDeparture(const TransferPlan* tp) {
    // Make sure this is called first on the leading ship

    if (GetCapableDV() <= tp->tot_dv || GetRemainingPayloadCapacity(tp->tot_dv) && tp->resource_transfer.quantity) {
        // Abort last minute
        // Untested!
        if (IsLeading()) {
            while (prepared_plans_count > 0) {
                RemoveTransferPlan(0);
            }
        } else {
            parent_obj = tp->departure_planet;
        }
        return;
    }

    PlanetaryEconomy* local_economy = &GetPlanet(tp->departure_planet)->economy;

    ResourceTransfer fuel_tf = local_economy->DrawResource(ResourceTransfer(RESOURCE_WATER, tp->fuel_mass));
    if (fuel_tf.quantity < tp->fuel_mass && IsPlayerFriend()) {
        resource_count_t remaining_fuel = tp->fuel_mass - fuel_tf.quantity;
        if (
            local_economy->trading_accessible && 
            local_economy->GetPrice(RESOURCE_WATER, remaining_fuel) < GlobalGetState()->money
        ) {
            USER_INFO("Automatically purchased %d of water for M§M %ld K", remaining_fuel, local_economy->GetPrice(RESOURCE_WATER, remaining_fuel) / 1e3)
            local_economy->TryPlayerTransaction(ResourceTransfer(RESOURCE_WATER, remaining_fuel));
            local_economy->DrawResource(ResourceTransfer(RESOURCE_WATER, remaining_fuel));
        } else {
            // Abort
            USER_INFO(
                "Not enough fuel. Could not afford/access remaining fuel %d cts on %s", 
                remaining_fuel, GetPlanet(tp->departure_planet)->name
            )
            local_economy->GiveResource(fuel_tf);
            prepared_plans_count = 0;
            return;
        }
    }

    transporing = local_economy->DrawResource(tp->resource_transfer);

    for(auto it = GlobalGetState()->quest_manager.active_tasks.GetIter(); it; it++) {
        if (GlobalGetState()->quest_manager.active_tasks[it]->ship == id) {
            GlobalGetState()->quest_manager.TaskDepartedFrom(it.GetId(), parent_obj);
        }
    }

    if (IsLeading()) {
        parent_obj = GetInvalidId();

        IDList following;
        GlobalGetState()->ships.GetFleet(&following, id);
        for (int i=0; i < following.size; i++) {
            GetShip(following[i])->_OnDeparture(tp);
        }
    }
    
    is_detected = true;

    Update();
}

void _OnFleetArrival(Ship* leading_ship, const TransferPlan* tp) {
    // Collects ships for combat
    IDList hostile_ships;
    uint32_t selection_flags = (~(1UL << leading_ship->allegiance) & 0xFF) | Ships::MILITARY_SELECTION_FLAG;
    GlobalGetState()->ships.GetOnPlanet(&hostile_ships, tp->arrival_planet, selection_flags);
    IDList allied_ships;
    GlobalGetState()->ships.GetFleet(&allied_ships, leading_ship->id);
    allied_ships.Append(leading_ship->id);
    if (hostile_ships.size > 0) {
        // First, military ships vs. military ships
        bool victory = ShipBattle(&allied_ships, &hostile_ships, Vector2Length(tp->arrival_dvs[tp->primary_solution]));
        // allied ship and hostile ship might contain dead ships
        if (victory) {
            uint32_t selection_flags = (~(1UL << leading_ship->allegiance) & 0xFF) | 0xFFFFFF00;
            IDList conquered;
            GlobalGetState()->ships.GetOnPlanet(&conquered, tp->arrival_planet, selection_flags);
            for(int i=0; i < conquered.size; i++) {
                GetShip(conquered[i])->allegiance = leading_ship->allegiance;
            }
        }
        for (int i=0; i < allied_ships.size; i++) {
            if (IsIdValid(allied_ships[i])) {
                GetShip(allied_ships[i])->is_detected = true;
            }
        }
    } else {
        uint32_t selection_flags = (~(1UL << leading_ship->allegiance) & 0xFF) | 0xFFFFFF00;
        IDList conquered;
        GlobalGetState()->ships.GetOnPlanet(&conquered, tp->arrival_planet, selection_flags);
        for(int i=0; i < conquered.size; i++) {
            GetShip(conquered[i])->allegiance = leading_ship->allegiance;
        }
        for (int i=0; i < allied_ships.size; i++) {
            GetShip(allied_ships[i])->is_detected = false;
        }
    }
}

void Ship::_OnArrival(const TransferPlan* tp) {
    // Make sure this is called first on the leading ship
    // First: Combat. Is the ship is intercepted when/before entering orbit, it can't do anything else

    if (IsLeading()) {
        _OnFleetArrival(this, tp);
        parent_obj = tp->arrival_planet;

        IDList following;
        GlobalGetState()->ships.GetFleet(&following, id);
        for (int i=0; i < following.size; i++) {
            GetShip(following[i])->_OnArrival(tp);
        }
    }

    GetPlanet(tp->arrival_planet)->economy.GiveResource(tp->resource_transfer);
    position = GetPlanet(GetParentPlanet())->position;

    // Complete tasks
    for(auto it = GlobalGetState()->quest_manager.active_tasks.GetIter(); it; it++) {
        const Task* quest = GlobalGetState()->quest_manager.active_tasks[it];
        if (quest->ship == id && quest->arrival_planet == tp->arrival_planet) {
            GlobalGetState()->quest_manager.TaskArrivedAt(it.GetId(), tp->arrival_planet);
        }
    }

    RemoveTransferPlan(0);
    Update();
}

void Ship::_EnsureContinuity() {
    INFO("Ensure Continuity Call");
    if (prepared_plans_count == 0) return;
    RID planet_tracker = parent_obj;
    int start_index = 0;
    if (!IsParked()) {
        planet_tracker = prepared_plans[0].arrival_planet;
        start_index = 1;
    }
    for(int i=start_index; i < prepared_plans_count; i++) {
        if (prepared_plans[i].departure_planet == planet_tracker) {
            planet_tracker = prepared_plans[i].arrival_planet;
        } else {
            RemoveTransferPlan(i);
        }
    }
}

Ships::Ships() {
    alloc.Init();

    ship_classes_ids = std::map<std::string, RID>();
    ship_classes = NULL;
    ship_classes_count = 0;
}

void Ships::ClearShips(){
    alloc.Clear();
}

RID Ships::AddShip(const DataNode* data) {
    Ship *ship;
    RID ship_entity;
    if (data->Has("id")) {
        RID rid = RID(data->GetI("id"));
        if(!alloc.AllocateAtID(rid, &ship)) {
            ERROR("INVALID while loading ship RID")
            return GetInvalidId();
        } 
    } else {
        ship_entity = alloc.Allocate(&ship);
    }
    
    ship->Deserialize(data);
    if (data->Has("planet")) {  // not necaissary strictly, but far more human-readable
        const char* planet_name = data->Get("planet");
        RID planet_id = GlobalGetState()->planets.GetIndexByName(planet_name);
        if (!IsIdValid(planet_id)) {
            FAIL("Error while initializing ship '%s': no such planet '%s'", ship->name, planet_name)
        }
        ship->parent_obj = planet_id;
    }

    ship->id = ship_entity;
    ship->Update();
    // If data doesn't specify variables, reset them to default (needs to be done after Update)
    if (!data->HasArray("variables")) {
        ship->Repair(1e6);
    }
    INFO("Spawned %s (%s-class)", ship->name, GetShipClassByIndex(ship->ship_class)->name);
    return ship_entity;
}

int Ships::LoadShipClasses(const DataNode* data) {
    if (ship_classes != NULL) {
        WARNING("Loading ship classes more than once (I'm not freeing this memory)");
    }
    ship_classes_count = data->GetArrayChildLen("ship_classes", true);
    if (ship_classes_count == 0){
        WARNING("No ship classes loaded")
        return 0;
    }
    ship_classes = new ShipClass[ship_classes_count];
    for (int index=0; index < ship_classes_count; index++) {
        const DataNode* ship_data = data->GetArrayChild("ship_classes", index);
        ShipClass sc = {0};

        strncpy(sc.name, ship_data->Get("name", "[NAME MISSING]"), SHIPCLASS_NAME_MAX_SIZE);
        strncpy(sc.description, ship_data->Get("description", "[DESCRITION MISSING]"), SHIPCLASS_DESCRIPTION_MAX_SIZE);

        sc.max_capacity = ship_data->GetI("capacity", 0);
        sc.max_dv = ship_data->GetF("dv", 0) * 1000;  // km/s -> m/s
        sc.v_e = ship_data->GetF("Isp", 0) * 1000;    // km/s -> m/s
        sc.construction_time = ship_data->GetI("construction_time", 20);
        sc.oem = ResourceCountsToKG(sc.max_capacity) / (exp(sc.max_dv/sc.v_e) - 1);
        sc.construction_batch_size = data->GetI("batch_size", 1, true);

        ship_data->FillBufferWithChild("construction_resources", sc.construction_resources, RESOURCE_MAX, resource_names);
        ship_data->FillBufferWithChild("stats", sc.stats, ShipStats::MAX, ship_stat_names);

        //ASSERT_ALOMST_EQUAL_FLOAT(sc.v_e * log((ResourceCountsToKG(sc.max_capacity) + sc.oem) / sc.oem), sc.max_dv)   // Remove when we're sure thisworks

        ship_classes[index] = sc;
        RID rid = RID(index, EntityType::SHIP_CLASS);
        auto pair = ship_classes_ids.insert_or_assign(ship_data->Get("id", "_"), rid);
        ship_classes[index].id = pair.first->first.c_str();  // points to string in dictionary
    }
    return ship_classes_count;
}

Ship* Ships::GetShip(RID id) const {
    if ((!alloc.ContainsID(id))) {
        FAIL("Invalid id (%d)", id)
    }
    return (Ship*) alloc.Get(id);
}

RID Ships::GetShipClassIndexById(const char *id) const { 
    auto find = ship_classes_ids.find(id);
    if (find == ship_classes_ids.end()) {
        ERROR("No such ship class id '%s'", id)
        return GetInvalidId();
    }
    return find->second;
}

void Ships::GetOnPlanet(IDList* list, RID planet, uint32_t allegiance_bits) const {
    for (auto it = alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        if (ship->IsParked() && planet != ship->GetParentPlanet()) 
            continue;
        if (!ship->IsParked() && planet != GetInvalidId()) 
            continue;
        if (((1 << ship->allegiance) & allegiance_bits) == 0) 
            continue;
        if (!(allegiance_bits & MILITARY_SELECTION_FLAG) && ship->GetCombatStrength() > 0) 
            continue;
        if (!(allegiance_bits & CIVILIAN_SELECTION_FLAG) && ship->GetCombatStrength() == 0) 
            continue;
        list->Append(it.GetId());
    }
}

void Ships::GetFleet(IDList *list, RID leading) const {
    for (auto it = alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        if (ship->parent_obj == leading) {
            list->Append(it.GetId());
        }
    }
}

void Ships::KillShip(RID uuid, bool notify_callback) {
    if (!IsIdValidTyped(uuid, EntityType::SHIP)) {
        return;
    }
    if (notify_callback) {
        GetWrenInterface()->NotifyShipEvent(uuid, "die");
    }
    alloc.Erase(uuid);
}

const ShipClass* Ships::GetShipClassByIndex(RID id) const {
    if (ship_classes == NULL) {
        ERROR("Ship Class uninitialized")
        return NULL;
    }
    if (IdGetType(id) != EntityType::SHIP_CLASS) {
        ERROR("id '%d' does not refer to a ship class", id)
        return NULL;
    }
    if (IdGetIndex(id) >= ship_classes_count) {
        ERROR("Invalid ship class index (%d >= %d or negative)", IdGetIndex(id), ship_classes_count)
        return NULL;
    }
    return &ship_classes[IdGetIndex(id)];
}

Ship* GetShip(RID uuid) {
    return GlobalGetState()->ships.GetShip(uuid);
}

const ShipClass* GetShipClassByIndex(RID index) {
    return GlobalGetState()->ships.GetShipClassByIndex(index);
}

int LoadShipClasses(const DataNode *data) {
    return GlobalGetState()->ships.LoadShipClasses(data);
}
