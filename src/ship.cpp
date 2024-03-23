#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"
#include "combat.hpp"

#include "render_utils.hpp"

double ShipClass::GetPayloadCapacityMass(double dv, int drop_tanks) const {
    //          dv_n = v_e * ln((max_cap + oem + extra_fuel * n) / (max_cap + oem + extra_fuel * (n - 1)))
    //          dv_0 = v_e * ln((max_cap + oem) / (x + eom))
    // <=> x + eom = (max_cap + oem + extra) / exp((dv - dv_extra)/v_e)

    const resource_count_t fuel_per_droptank = 10;
    
    return (ResourceCountsToKG(max_capacity + fuel_per_droptank * drop_tanks) + oem) / exp(dv/v_e) - oem;
}

resource_count_t ShipClass::GetFuelRequiredFull(double dv, int drop_tanks) const {
    // assuming max payload
    const resource_count_t fuel_per_droptank = 10;
    double extra_fuel = fuel_per_droptank * drop_tanks;
    return max_capacity + extra_fuel - KGToResourceCounts(GetPayloadCapacityMass(dv, extra_fuel));
}

resource_count_t ShipClass::GetFuelRequiredEmpty(double dv) const {
    // assuming no payload
    return KGToResourceCounts(oem * (exp(dv/v_e) - 1));
}

void ShipClass::MouseHintWrite(StringBuilder* sb) const {
    sb->Add(name).Add("\n");
    sb->Add(description).Add("\n");

    StringBuilder sb2;
    int stat_num = 0;
    for (int i=0; i < ship_stats::MAX; i++) {
        if (stats[i] > 0) {
            sb2.AddFormat("%s %3d  ", ship_stats::icons[i], stats[i]);
            stat_num++;
        }
    }
    if (stat_num > 0) {
        sb->Add("Base: ").Add(sb2.c_str).Add("\n");
    }
}

Ship::Ship() {
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }
}

Ship::~Ship() {
    GetRenderServer()->icons.EraseAt(icon3d);
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
    if (GetGlobalState()->focused_ship == id) {
        GetCamera()->focus_object = id;
    } else {
        GetGlobalState()->focused_ship = id;
    }
    HandleButtonSound(ButtonStateFlags::JUST_PRESSED);
}

void Ship::_OnNewPlanClicked() {
    TransferPlanUI& tp_ui = GetGlobalState()->active_transfer_plan;
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

void Ship::Serialize(DataNode *data) const {
    data->Set("name", name);
    data->SetI("allegiance", allegiance);
    
    data->Set("class_id", GetShipClassByRID(ship_class)->id);
    data->SerializeBuffer("transporting", transporting, resources::names, resources::MAX);
    data->SerializeBuffer("dammage_taken", dammage_taken, ship_variables::names, ship_variables::MAX);

    data->CreateArray("modules", SHIP_MAX_MODULES);
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) {
            data->InsertIntoArray("modules", i, GetModule(modules[i])->id);
        } else {
            data->InsertIntoArray("modules", i, "---");
        }
    }

    data->CreatChildArray("prepared_plans", prepared_plans_count);
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i].Serialize(data->InsertIntoChildArray("prepared_plans", i));
    }
}

void Ship::Deserialize(const DataNode* data) {
    strcpy(name, data->Get("name", "UNNAMED"));
    allegiance = data->GetI("allegiance", allegiance);
    plan_edit_index = -1;
    ship_class = GetShips()->GetShipClassIndexById(data->Get("class_id"));
    if (!IsIdValid(ship_class)) {
        FAIL("Invalid ship class")  // TODO fail more gracefully
    }
    data->DeserializeBuffer("transporting", transporting, resources::names, resources::MAX);

    int modules_count = data->GetArrayLen("modules", true);
    if (modules_count > SHIP_MAX_MODULES) modules_count = SHIP_MAX_MODULES;
    const ShipModules* sms = GetShipModules();
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }
    for(int i=0; i < modules_count; i++) {
        RID module_rid = sms->GetModuleRIDFromStringId(data->GetArrayElem("modules", i));
        if (!IsIdValidTyped(module_rid, EntityType::MODULE_CLASS)) continue;
        module_types::T module_type = sms->GetModuleByRID(module_rid)->type;
        int proper_index = GetFreeModuleSlot(module_type).index;
        if (proper_index < 0) continue;
        modules[proper_index] = module_rid;
    }

    prepared_plans_count = data->GetChildArrayLen("prepared_plans", true);
    if (prepared_plans_count > SHIP_MAX_PREPARED_PLANS) prepared_plans_count = SHIP_MAX_PREPARED_PLANS;
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i] = TransferPlan();
        prepared_plans[i].Deserialize(data->GetChildArrayElem("prepared_plans", i, true));
    }

    data->DeserializeBuffer("dammage_taken", dammage_taken, ship_variables::names, ship_variables::MAX);
}

double Ship::GetOperationalMass() const {  // Without resources
    double res = 0.0;

    QuestManager* qm = GetQuestManager();
    for(auto i = qm->active_tasks.GetIter(); i; i++) {
        if (qm->active_tasks[i]->ship == id) {
            res += qm->active_tasks[i]->payload_mass;
        }
    }

    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i]) && modules[i] != GetShipModules()->expected_modules.droptank) {
            res += GetModule(modules[i])->mass;
        }
    }

    return res;
}

resource_count_t Ship::GetMaxCapacity() const {
    return GetShipClassByRID(ship_class)->max_capacity;
}

resource_count_t Ship::GetRemainingPayloadCapacity(double dv) const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountModulesOfClass(GetShipModules()->expected_modules.droptank);
    double capacity = sc->GetPayloadCapacityMass(dv, drop_tanks);
    return KGToResourceCounts(capacity - GetOperationalMass());
}

resource_count_t Ship::GetFuelRequiredFull(double dv) const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountModulesOfClass(GetShipModules()->expected_modules.droptank);
    const resource_count_t fuel_per_droptank = 10;
    double extra_fuel = fuel_per_droptank * drop_tanks;
    return sc->max_capacity + extra_fuel - KGToResourceCounts(sc->GetPayloadCapacityMass(dv, extra_fuel));
}

resource_count_t Ship::GetFuelRequired(double dv, resource_count_t payload) const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountModulesOfClass(GetShipModules()->expected_modules.droptank);
    const resource_count_t fuel_per_droptank = 10;
    double extra_fuel = fuel_per_droptank * drop_tanks;
    double dry_mass = sc->oem + GetOperationalMass() + ResourceCountsToKG(payload);
    return KGToResourceCounts((exp(dv/sc->v_e) - 1) * dry_mass - extra_fuel);
}

double Ship::GetCapableDV() const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountModulesOfClass(GetShipModules()->expected_modules.droptank);

    const resource_count_t fuel_per_droptank = 10;
    
    return sc->v_e * log((ResourceCountsToKG(sc->max_capacity + fuel_per_droptank * drop_tanks) + sc->oem) / (GetOperationalMass() + sc->oem));

    // max_dv = v_e * log(max_capacity / oem + 1)
    // exp(max_dv / v_e) - 1

    // oem = max_capacity / (exp(max_dv/v_e) - 1);
    // sc->v_e * log(max_capacity / oem + 1);
}

bool Ship::IsPlayerControlled() const {
    return allegiance == GetFactions()->player_faction;
}

IntelLevel::T Ship::GetIntelLevel() const {
    return IntelLevel::FULL;

    if (IsPlayerControlled()) {
        return IntelLevel::FULL;
    }
    return IntelLevel::STATS | (is_detected * IntelLevel::TRAJECTORY);
}

bool Ship::IsTrajectoryKnown(int index) const {
    return true;
    if (IsPlayerControlled()) {
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
    //SHOW_I(stats[ship_stats::KINETIC_HP])
    //SHOW_I(stats[ship_stats::ENERGY_HP])
    //SHOW_I(stats[ship_stats::CREW])
    //SHOW_I(variables[ship_variables::KINETIC_ARMOR])
    //SHOW_I(variables[ship_variables::ENERGY_ARMOR])
    //SHOW_I(variables[ship_variables::CREW])
    
    return
          dammage_taken[ship_variables::KINETIC_ARMOR]
        + dammage_taken[ship_variables::ENERGY_ARMOR]
        + dammage_taken[ship_variables::CREW]
    ;
}

const char* _GetTypeIcon(ShipType::T ship_type) {
    switch (ship_type) {
        case ShipType::SHIPYARD: return " " ICON_STATION;
        case ShipType::UTILITY: return " " ICON_UTIL_SHIP;
        case ShipType::TRANSPORT: return " " ICON_TRANSPORT_SHIP;
        case ShipType::MILITARY: return " " ICON_MIL_SHIP;
    }
    return ICON_EMPTY;
}

const char *Ship::GetTypeIcon() const {
    return _GetTypeIcon(GetShipType());
}

bool Ship::CanDragModule(int index) const {
    if (!IsIdValid(modules[index]))
        return true;
    if (GetModule(modules[index])->delta_stats[ship_stats::KINETIC_HP] > kinetic_hp() - dammage_taken[ship_variables::KINETIC_ARMOR]) {
        return false;
    }
    if (GetModule(modules[index])->delta_stats[ship_stats::ENERGY_HP] > energy_hp() - dammage_taken[ship_variables::ENERGY_ARMOR]) {
        return false;
    }
    if (GetModule(modules[index])->delta_stats[ship_stats::CREW] > crew() - dammage_taken[ship_variables::CREW]) {
        return false;
    }
    return true;
}

Color Ship::GetColor() const {
    return IsPlayerControlled() ? Palette::ally : Palette::enemy;
}

bool Ship::IsStatic() const {
    return GetShipClassByRID(ship_class)->max_dv == 0;
}

bool Ship::IsParked() const {
    switch (IdGetType(parent_obj)) {
    case EntityType::PLANET: return true;
    case EntityType::INVALID: return false;
    case EntityType::SHIP: return GetShip(parent_obj)->IsParked();
    default: NOT_REACHABLE
    }
    return false;  // To satisfy warning
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
    return GetInvalidId();  // To satisfy warning
}

int Ship::CountModulesOfClass(RID module_class) const {
    int res = 0;
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (modules[i] == module_class) res++;
    }
    return res;
}

ShipType::T Ship::GetShipType() const {
    if (IsStatic()) return ShipType::SHIPYARD;
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        //if (GetShipModules()->GetModuleByRID(modules[i])->production) return ShipType::UTILITY;
        // TODO: when is it 'UTILITY'?
    }
    if (GetCombatStrength() > 0) return ShipType::MILITARY;
    return ShipType::TRANSPORT;
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
    double available_dv = GetCapableDV();
    if (tp->tot_dv > GetCapableDV()) {
        ERROR("Not enough DV %f > %f", tp->tot_dv, GetCapableDV());
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
    TransferPlanUI& tp_ui = GetGlobalState()->active_transfer_plan;
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
        position = GetPlanet(GetParentPlanet())->position;
    } else {
        const TransferPlan* tp = &prepared_plans[0];
        if (IsParked()) {
            if (tp->departure_time < now) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(GetParentPlanet())->position;
            }
        } else {
            if (tp->arrival_time < now) {
                _OnArrival(tp);
            } else {
                position = tp->transfer_orbit[tp->primary_solution].GetPosition(now);
            }
        }
    }

    _UpdateModules();
    _UpdateShipyard();
}

void Ship::_UpdateShipyard() {
    int collected_components[4] = {0};
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        if(modules[i] == GetShipModules()->expected_modules.small_yard_1) collected_components[0]++;
        if(modules[i] == GetShipModules()->expected_modules.small_yard_2) collected_components[1]++;
        if(modules[i] == GetShipModules()->expected_modules.small_yard_3) collected_components[2]++;
        if(modules[i] == GetShipModules()->expected_modules.small_yard_4) collected_components[3]++;
    }
    int module_factories = MinInt(collected_components[1], collected_components[2]);
    int repair_stations = MinInt(collected_components[2], collected_components[3]);
    int ship_yards = MinInt(
        MinInt(collected_components[0], collected_components[1]),
        repair_stations
    );
    if (GetCalendar()->IsNewDay() && IsParked()) {
        for(int i=0; i < repair_stations; i++) {
            IDList available_ships;
            uint32_t filter = ((1 << allegiance) & 0xFFU) | 0xFFFFFF00U;
            GetShips()->GetOnPlanet(&available_ships, parent_obj, filter);
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
            GetPlanet(GetParentPlanet())->AdvanceModuleProductionQueue();
        }
        for(int i=0; i < ship_yards; i++) {
            GetPlanet(GetParentPlanet())->AdvanceShipProductionQueue();
        }
    }
}

void Ship::_UpdateModules() {
    // Recalculate every frame

    // Gotta exist a more efficient way
    // And robust. Dependency graph (also a way to find circular dependencies)

    for (int i=0; i < ship_stats::MAX; i++) {
        stats[i] = GetShipClassByRID(ship_class)->stats[i];
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

void Ship::DrawIcon(int x_offsets[], int y_offsets[], float grow_factor) {
    // Called in normal 3D render mode
    
    float stride = Lerp(8.0f, 20.0f, grow_factor);
    float icon_size = Lerp(8.0f, 18.0f, grow_factor);
    int type = GetShipType();
    Vector2 draw_offset = { x_offsets[type], y_offsets[type] };
    y_offsets[type] += stride;
    if (type == 0 || type == 3) draw_offset.y = -draw_offset.y;

    draw_pos = Vector2Add(GetCamera()->GetScreenPos(position.cartesian), draw_offset);
    
    if (!IsIdValidTyped(icon3d, EntityType::ICON3D)) {
        icon3d = GetRenderServer()->icons.Allocate();
    }
    if (!IsIdValidTyped(text3d, EntityType::TEXT3D)) {
        text3d = GetRenderServer()->text_labels_3d.Allocate();
    }

    Icon3D* icon_inst = GetRenderServer()->icons.Get(icon3d);
    icon_inst->scale = icon_size;
    icon_inst->offset = draw_offset;
    icon_inst->atlas_pos = AtlasPos(type, 28);
    icon_inst->color = GetColor();
    icon_inst->world_pos = position.cartesian;
    Vector3 from = GameCamera::WorldToRender(position.cartesian);
    Vector3 to = icon_inst->GetFinalRenderPos();

    if (GetCamera()->IsInView(from) && GetCamera()->IsInView(to)) {
        DrawLine3D(from, to, GetColor());
    }
    
    Text3D* text_inst = GetRenderServer()->text_labels_3d.Get(text3d);
    text_inst->scale = DEFAULT_FONT_SIZE;
    text_inst->offset = draw_offset;
    if (type < 2) {
        text_inst->offset.x -= icon_size - 3;
    } else {
        text_inst->offset.x += icon_size + 3;
    }
    text_inst->text = name;
    text_inst->color = GetColor();
    text_inst->color.a = grow_factor * 255;
    text_inst->world_pos = position.cartesian;
    text_inst->alignment = (type < 2 ? TextAlignment::RIGHT : TextAlignment::LEFT) | TextAlignment::VCENTER;

    //DrawLineV(draw_pos, Vector2Add(draw_pos, draw_offset), GetColor());
    
    //DrawRectangleV(Vector2SubtractValue(draw_pos, 4.0f), {8, 8}, color);
}

void Ship::DrawTrajectories() const {
    Color color = GetColor();
    
    /*if (plan_edit_index >= 0 
        && IsIdValid(prepared_plans[plan_edit_index].arrival_planet)
        && IsTrajectoryKnown(plan_edit_index)
    ) {
        const TransferPlan* last_tp = &prepared_plans[plan_edit_index];
        OrbitPos last_pos = GetPlanet(last_tp->arrival_planet)->orbit.GetPosition(
            last_tp->arrival_time
        );
        //Vector2 last_draw_pos = GetCamera()->GetScreenPos(last_pos.cartesian);
        DrawIcon(last_pos.cartesian, GetShipType(), ColorAlpha(color, 0.5), Vector2Zero());
    }*/
    

    if ((GetIntelLevel() & IntelLevel::TRAJECTORY) == 0) {
        return;
    }

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

        OrbitSegment tf_orbit = OrbitSegment(&plan->transfer_orbit[plan->primary_solution], to_departure, to_arrival);
        //RenderOrbit(&tf_orbit, 256, i == plan_edit_index ? Palette::ui_main : GetColor());
        //OrbitSegment tf_orbit = OrbitSegment(&plan->transfer_orbit[plan->primary_solution]);
        RenderOrbit(&tf_orbit, 256, OrbitRenderMode::Solid, color);
    }
}

void _UIDrawHeader(const Ship* ship) {
    ui::WriteEx(ship->name, TextAlignment::CONFORM, false);  
    ui::WriteEx(ship->GetTypeIcon(), TextAlignment::CONFORM, false);
    const char* text = ICON_EMPTY " " ICON_EMPTY "  ";  // use ICON_EMPTY instead of space to get the right spacing
    if (!ship->IsParked() && !ship->IsLeading()) text = ICON_EMPTY " " ICON_TRANSPORT_FLEET "  ";
    if (ship->IsParked() && ship->IsLeading()) text = ICON_PLANET " " ICON_EMPTY "  ";
    if (ship->IsParked() && !ship->IsLeading()) text = ICON_PLANET " " ICON_TRANSPORT_FLEET "  ";
    text::Layout layout = ui::Current()->GetTextLayout(text, TextAlignment::RIGHT | TextAlignment::VCONFORM);
    int pos = layout.GetCharacterIndex(GetMousePosition());
    ui::WriteEx(text, TextAlignment::RIGHT | TextAlignment::VCONFORM, true);
    ButtonStateFlags::T planet_button_state = GetButtonState(pos >= 0 && pos < 2, false);  // Don't care about hover_in
    ButtonStateFlags::T fleet_button_state = GetButtonState(pos >= 3 && pos < 5, false);  // Don't care about hover_in
    if (planet_button_state & ButtonStateFlags::JUST_PRESSED && ship->IsParked()) {
        GetGlobalState()->focused_planet = ship->GetParentPlanet();
    }
    if (fleet_button_state & ButtonStateFlags::JUST_PRESSED && !ship->IsLeading()) {
        GetGlobalState()->focused_ship = ship->parent_obj;
    }
    
    if (planet_button_state & ButtonStateFlags::HOVER && ship->IsParked()) {
        ui::SetMouseHint("Select parent planet");
    }
    if (fleet_button_state & ButtonStateFlags::HOVER && !ship->IsLeading()) {
        ui::SetMouseHint("Select leading ship");
    }
    ui::VSpace(10);
}

void _UIDrawStats(const Ship* ship) {
    StringBuilder sb;
    sb.AddFormat(ICON_PAYLOAD " %d / %d ", KGToResourceCounts(ship->GetOperationalMass()), ship->GetMaxCapacity());
    ui::WriteEx(sb.c_str, TextAlignment::LEFT | TextAlignment::VCONFORM, false);
    sb.Clear();
    if (ship->IsStatic()) {
        sb.AddFormat("\u0394V --       ", ship->GetCapableDV() / 1000.f);
    } else {
        sb.AddFormat("\u0394V %2.2f km/s  ", ship->GetCapableDV() / 1000.f);
    }
    ui::WriteEx(sb.c_str, TextAlignment::RIGHT | TextAlignment::VCONFORM, false);
    ui::Fillline(ship->GetOperationalMass() / ResourceCountsToKG(ship->GetMaxCapacity()), Palette::ui_main, Palette::bg);
    ui::Write("");  // Linebreak
    ui::VSpace(15);
    ui::PushInset(0, 4 * 20-8);
    sb.Clear();
    sb.AddFormat(ICON_POWER "%2d" ICON_ACS "%2d\n", ship->power(), ship->initiative());
    sb.AddFormat(" %2d/%2d" ICON_HEART_KINETIC "  %2d/%2d" ICON_HEART_ENERGY "  %3d/%3d" ICON_HEART_BOARDING "\n", 
        ship->kinetic_hp() - ship->dammage_taken[ship_variables::KINETIC_ARMOR], ship->kinetic_hp(),
        ship->energy_hp() - ship->dammage_taken[ship_variables::ENERGY_ARMOR], ship->energy_hp(),
        ship->crew() - ship->dammage_taken[ship_variables::CREW], ship->crew()
    );
    sb.AddFormat("   %2d " ICON_ATTACK_KINETIC "    %2d " ICON_ATTACK_ORDNANCE "     %3d " ICON_ATTACK_BOARDING "\n", ship->kinetic_offense(), ship->ordnance_offense(), ship->boarding_offense());
    sb.AddFormat("   %2d " ICON_SHIELD_KINETIC "    %2d " ICON_SHIELD_ORDNANCE "     %3d " ICON_SHIELD_BOARDING "\n", ship->kinetic_defense(), ship->ordnance_defense(), ship->boarding_defense());
    //sb.AddFormat("++++++++++++++++++");
    ui::Write(sb.c_str);
    ui::HelperText(GetUI()->GetConceptDescription("stat"));
    ui::Pop();  // Inset
}

void _UIDrawTransferplans(Ship* ship) {
    int inset_height = ship->prepared_plans_count * (ui::Current()->GetLineHeight() * 2 + 8 + 4);
    inset_height += 20;

    ui::PushInset(0, inset_height);
    ui::VSpace(20);
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
        ui::PushInset(2, ui::Current()->GetLineHeight() * 2);
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            ui::EncloseEx(4, Palette::bg, Palette::interactable_main, 4);
        } else {
            ui::Enclose();
        }
        ui::PushHSplit(0, -32);
        ui::Write(sb.c_str);
        HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
        if (button_state & ButtonStateFlags::HOVER) {
            ship->highlighted_plan_index = i;
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            ship->StartEditingPlan(i);
        }
        ui::Pop();  // HSplit

        if (i != ship->plan_edit_index) {
            ui::PushHSplit(-32, -1);
            ButtonStateFlags::T button_state = ui::AsButton();
            if (button_state & ButtonStateFlags::HOVER) {
                ui::EncloseEx(4, Palette::interactable_alt, Palette::interactable_alt, 4);
            } else {
                ui::EncloseEx(4, Palette::bg, Palette::bg, 4);
            }
            ui::WriteEx(ICON_X, TextAlignment::CENTER, false);
            HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
            if (button_state & ButtonStateFlags::JUST_PRESSED) {
                ship->RemoveTransferPlan(i);
            }
            ui::Pop();  // HSplit
        }

        ui::Pop();  // Insert
    }
    ui::HelperText(GetUI()->GetConceptDescription("transfer"));
    ui::Pop();  // Inset
    
    // 'New transfer' button

    Rectangle text_rect = ui::Current()->TbMeasureText("New transfer", TextAlignment::CONFORM);
    ui::PushInline(text_rect.width + 2, text_rect.height + 2);
    ButtonStateFlags::T new_button_state = ui::AsButton();
    if (new_button_state & ButtonStateFlags::HOVER) {
        ui::EnclosePartial(0, Palette::bg, Palette::interactable_main, Direction::DOWN);
    } else {
        ui::EnclosePartial(0, Palette::bg, Palette::ui_main, Direction::DOWN);
    }
    ui::WriteEx("New transfer", TextAlignment::CENTER, false);
    HandleButtonSound(new_button_state);
    ui::Pop();  // Inline
    if (new_button_state & ButtonStateFlags::JUST_PRESSED) {
        ship->_OnNewPlanClicked();
    }
}

void _UIDrawFleet(Ship* ship) {
    // Following
    if (!ship->IsLeading()) {
        ui::PushInset(0, DEFAULT_FONT_SIZE+4);
        StringBuilder sb;
        if (ship->IsParked()) {
            sb.Add("Detach from").Add(GetShip(ship->parent_obj)->name);
            ButtonStateFlags::T button_state = ui::DirectButton(sb.c_str, 0);
            HandleButtonSound(button_state);
            if (button_state & ButtonStateFlags::JUST_PRESSED) {
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
    uint32_t selection_flags = ((1UL << ship->allegiance) & 0xFF) | 0xFFFFFF00;
    GetShips()->GetOnPlanet(&candidates, ship->GetParentPlanet(), selection_flags);
    for (int i=0; i < candidates.size; i++) {
        if (candidates[i] == ship->id) continue;
        const Ship* candidate = GetShip(candidates[i]);
        if (!candidate->IsLeading()) continue;
        ui::PushInset(0, DEFAULT_FONT_SIZE + 4);
        ui::WriteEx("Attach to ", TextAlignment::CONFORM, false);
        ui::Write(candidate->name);
        ButtonStateFlags::T button_state = ui::AsButton();
        HandleButtonSound(button_state);
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
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

    const int INSET_MARGIN = 6;
    const int OUTSET_MARGIN = 6;
    const int TEXT_SIZE = DEFAULT_FONT_SIZE;
    int width = (SHIP_MODULE_WIDTH + 3) * 6 + 9 + 8 + 3;
    ui::CreateNew(
        GetScreenWidth() - width - 2*INSET_MARGIN - OUTSET_MARGIN, 
        OUTSET_MARGIN + 200,
        width + 2*INSET_MARGIN, 
        GetScreenHeight() - 200 - OUTSET_MARGIN - 2*INSET_MARGIN,
        TEXT_SIZE, Palette::ui_main, Palette::bg
    );
    ui::EncloseEx(4, Palette::bg, GetColor(), 4);
    ui::Shrink(INSET_MARGIN, INSET_MARGIN);

    if ((GetIntelLevel() & IntelLevel::STATS) == 0) {
        return;
    }

    const int allocated = 1000;

    ui::PushScrollInset(0, ui::Current()->height, allocated, &ui_scroll);

    _UIDrawHeader(this);

    _UIDrawStats(this);
    //if (!IsPlayerControlled()) {
    //    ui::Pop();  // ScrollInset
    //    return;
    //}
    GetShipClassByRID(ship_class)->module_config.Draw(this);
    if (!IsStatic()) {
        _UIDrawTransferplans(this);
        _UIDrawFleet(this);
        _UIDrawQuests(this);
    }

    ui::Pop();  // ScrollInset
    ui::HelperText(GetUI()->GetConceptDescription("ship"));
}

void Ship::Inspect() {

}

void Ship::RemoveShipModuleAt(int index) {
    modules[index] = GetInvalidId();
}

void Ship::Repair(int hp) {
    if (hp > stats[ship_stats::KINETIC_HP] + stats[ship_stats::ENERGY_HP] + stats[ship_stats::CREW]){
        // for initializetion
        dammage_taken[ship_variables::KINETIC_ARMOR] = 0;
        dammage_taken[ship_variables::ENERGY_ARMOR] = 0;
        dammage_taken[ship_variables::CREW] = 0;
        return;
    }
    int kinetic_repair = MinInt(hp, dammage_taken[ship_variables::KINETIC_ARMOR]);
    hp -= kinetic_repair;
    int energy_repair = MinInt(hp, dammage_taken[ship_variables::ENERGY_ARMOR]);
    hp -= energy_repair;
    int crew_repair = MinInt(hp, dammage_taken[ship_variables::CREW]);
    dammage_taken[ship_variables::KINETIC_ARMOR] -= kinetic_repair;
    dammage_taken[ship_variables::ENERGY_ARMOR] -= energy_repair;
    dammage_taken[ship_variables::CREW] -= crew_repair;
}

ShipModuleSlot Ship::GetFreeModuleSlot(module_types::T least) const {
    const ModuleConfiguration* module_config = &GetShipClassByRID(ship_class)->module_config;
    if (least == module_types::FREE) {
        for(int index = module_config->module_count; index < SHIP_MAX_MODULES; index++) {
            if(!IsIdValid(modules[index])) {
                return ShipModuleSlot(id, index, ShipModuleSlot::DRAGGING_FROM_SHIP, module_types::FREE);
            }
        }
        return ShipModuleSlot(id, -1, ShipModuleSlot::DRAGGING_FROM_SHIP, module_types::INVALID);
    }
    for (int index = 0; index < module_config->module_count; index++) {
        if(!IsIdValid(modules[index]) && module_types::IsCompatible(least, module_config->types[index])) {
            return ShipModuleSlot(id, index, ShipModuleSlot::DRAGGING_FROM_SHIP, module_config->types[index]);
        }
    }
    return ShipModuleSlot(id, -1, ShipModuleSlot::DRAGGING_FROM_SHIP, module_types::INVALID);
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

    resource_count_t tot_payload = tp->GetPayloadMass();
    if (GetCapableDV() < tp->tot_dv || GetRemainingPayloadCapacity(tp->tot_dv) < tot_payload) {
        // Abort last minute
        INFO("DV: %f < %f, or", GetCapableDV(), tp->tot_dv)
        INFO("payload: %d < %d", GetRemainingPayloadCapacity(tp->tot_dv), tot_payload)
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
    //resources::T fuel_type = GetShipClassByRID(ship_class)->fuel_resource;

    resource_count_t fuel_quantity = local_economy->TakeResource(tp->fuel_type, tp->fuel);
    if (fuel_quantity < tp->fuel && IsPlayerControlled()) {
        resource_count_t remaining_fuel = tp->fuel - fuel_quantity;
        if (
            local_economy->trading_accessible && 
            local_economy->GetPrice(tp->fuel_type, remaining_fuel) < GetFactions()->GetMoney(allegiance)
        ) {
            USER_INFO("Automatically purchased %d of water for M§M %d K", remaining_fuel, local_economy->GetPrice(tp->fuel_type, remaining_fuel) / 1e3)
            local_economy->TryPlayerTransaction(tp->fuel_type, fuel_quantity);
            local_economy->TakeResource(tp->fuel_type, fuel_quantity);
        } else {
            // Abort
            USER_INFO(
                "Not enough fuel. Could not afford/access remaining fuel %d cts on %s", 
                remaining_fuel, GetPlanet(tp->departure_planet)->name
            )
            local_economy->GiveResource(tp->fuel_type, fuel_quantity);
            prepared_plans_count = 0;
            return;
        }
    }

    for (int i=0; i < resources::MAX; i++) {
        transporting[i] = local_economy->TakeResource((resources::T) i, tp->resource_transfer[i]);
    }
    //INFO("%s: %d", resources::names[transporing.resource_id], transporing.quantity)

    for(auto it = GetQuestManager()->active_tasks.GetIter(); it; it++) {
        if (GetQuestManager()->active_tasks[it]->ship == id) {
            GetQuestManager()->TaskDepartedFrom(it.GetId(), parent_obj);
        }
    }

    if (IsLeading()) {
        parent_obj = GetInvalidId();

        IDList following;
        GetShips()->GetFleet(&following, id);
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
    GetShips()->GetOnPlanet(&hostile_ships, tp->arrival_planet, selection_flags);
    IDList allied_ships;
    GetShips()->GetFleet(&allied_ships, leading_ship->id);
    allied_ships.Append(leading_ship->id);
    if (hostile_ships.size > 0) {
        // First, military ships vs. military ships
        bool victory = ShipBattle(&allied_ships, &hostile_ships, tp->arrival_dvs[tp->primary_solution].Length());
        // allied ship and hostile ship might contain dead ships
        if (victory) {
            uint32_t selection_flags = (~(1UL << leading_ship->allegiance) & 0xFF) | 0xFFFFFF00;
            IDList conquered;
            GetShips()->GetOnPlanet(&conquered, tp->arrival_planet, selection_flags);
            for(int i=0; i < conquered.size; i++) {
                GetShip(conquered[i])->allegiance = leading_ship->allegiance;
            }
            GetPlanet(tp->arrival_planet)->Conquer(leading_ship->allegiance, false);
        }
        for (int i=0; i < allied_ships.size; i++) {
            if (IsIdValid(allied_ships[i])) {
                GetShip(allied_ships[i])->is_detected = true;
            }
        }
    } else {
        uint32_t selection_flags = (~(1UL << leading_ship->allegiance) & 0xFF) | 0xFFFFFF00;
        IDList conquered;
        GetShips()->GetOnPlanet(&conquered, tp->arrival_planet, selection_flags);
        for(int i=0; i < conquered.size; i++) {
            GetShip(conquered[i])->allegiance = leading_ship->allegiance;
        }
        for (int i=0; i < allied_ships.size; i++) {
            GetShip(allied_ships[i])->is_detected = false;
        }
        GetPlanet(tp->arrival_planet)->Conquer(leading_ship->allegiance, false);
    }
}

void Ship::_OnArrival(const TransferPlan* tp) {
    // Make sure this is called first on the leading ship
    // First: Combat. Is the ship is intercepted when/before entering orbit, it can't do anything else

    if (IsLeading()) {
        _OnFleetArrival(this, tp);
        parent_obj = tp->arrival_planet;

        IDList following;
        GetShips()->GetFleet(&following, id);
        for (int i=0; i < following.size; i++) {
            GetShip(following[i])->_OnArrival(tp);
        }
    }

    int total_resources = 0;
    for (int i=0; i < resources::MAX; i++) {
        GetPlanet(tp->arrival_planet)->economy.GiveResource((resources::T) i, transporting[i]);
        total_resources += transporting[i];
    }
    position = GetPlanet(GetParentPlanet())->position;

    // Complete tasks
    for(auto it = GetQuestManager()->active_tasks.GetIter(); it; it++) {
        const Task* quest = GetQuestManager()->active_tasks[it];
        if (quest->ship == id && quest->arrival_planet == tp->arrival_planet) {
            GetQuestManager()->TaskArrivedAt(it.GetId(), tp->arrival_planet);
        }
    }

    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (modules[i] == GetShipModules()->expected_modules.droptank) {
            RemoveShipModuleAt(i);
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
        ship_entity = RID(data->GetI("id"));
        if(!alloc.AllocateAtID(ship_entity, &ship)) {
            ERROR("INVALID while loading ship RID")
            return GetInvalidId();
        } 
    } else {
        ship_entity = alloc.Allocate(&ship);
    }
    
    ship->Deserialize(data);
    if (data->Has("planet")) {  // not necaissary strictly, but far more human-readable
        const char* planet_name = data->Get("planet");
        RID planet_id = GetPlanets()->GetIdByName(planet_name);
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
    //INFO("Spawned %s (%s-class)", ship->name, GetShipClassByRID(ship->ship_class)->name);
    return ship_entity;
}

int Ships::LoadShipClasses(const DataNode* data) {
    if (ship_classes != NULL) {
        WARNING("Loading ship classes more than once (I'm not freeing this memory)");
    }
    ship_classes_count = data->GetChildArrayLen("ship_classes", true);
    if (ship_classes_count == 0){
        WARNING("No ship classes loaded")
        return 0;
    }
    const DataNode* module_configurations = data->GetChild("module_configurations");
    ship_classes = new ShipClass[ship_classes_count];
    for (int index=0; index < ship_classes_count; index++) {
        const DataNode* sc_data = data->GetChildArrayElem("ship_classes", index);
        ShipClass sc = {0};

        strncpy(sc.name, sc_data->Get("name", "[NAME MISSING]"), SHIPCLASS_NAME_MAX_SIZE);
        strncpy(sc.description, sc_data->Get("description", "[DESCRITION MISSING]"), SHIPCLASS_DESCRIPTION_MAX_SIZE);
        const char* ship_id = sc_data->Get("id", "_");

        sc.max_capacity = sc_data->GetI("capacity", 0);
        sc.max_dv = sc_data->GetF("dv", 0) * 1000;  // km/s -> m/s
        sc.v_e = sc_data->GetF("Isp", 0) * 1000;    // km/s -> m/s
        sc.fuel_resource = (resources::T) FindResource(sc_data->Get("fuel", "", true), resources::WATER);
        sc.construction_time = sc_data->GetI("construction_time", 20);
        sc.oem = ResourceCountsToKG(sc.max_capacity) / (exp(sc.max_dv/sc.v_e) - 1);
        sc.construction_batch_size = sc_data->GetI("batch_size", 1, true);
        sc.is_hidden = strcmp(sc_data->Get("hidden", "n", true), "y") == 0;
        sc.icon_index = AtlasPos(
            sc_data->GetArrayElemI("icon_index", 0, 31),
            sc_data->GetArrayElemI("icon_index", 1, 31));
        sc.module_config.Load(module_configurations, ship_id);

        sc_data->DeserializeBuffer("construction_resources", sc.construction_resources, resources::names, resources::MAX);
        sc_data->DeserializeBuffer("stats", sc.stats, ship_stats::names, ship_stats::MAX);

        //ASSERT_ALOMST_EQUAL_FLOAT(sc.v_e * log((ResourceCountsToKG(sc.max_capacity) + sc.oem) / sc.oem), sc.max_dv)   // Remove when we're sure thisworks

        ship_classes[index] = sc;
        RID rid = RID(index, EntityType::SHIP_CLASS);
        auto pair = ship_classes_ids.insert_or_assign(ship_id, rid);
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
    alloc.EraseAt(uuid);
}

const ShipClass* Ships::GetShipClassByRID(RID id) const {
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
    return GetShips()->GetShip(uuid);
}

const ShipClass* GetShipClassByRID(RID index) {
    return GetShips()->GetShipClassByRID(index);
}

int LoadShipClasses(const DataNode *data) {
    return GetShips()->LoadShipClasses(data);
}
