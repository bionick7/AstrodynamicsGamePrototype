#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"
#include "combat.hpp"

#include <bitset>

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

void ShipClass::MouseHintWrite() const {
    ui::WriteEx(name.GetChar(), text_alignment::HCENTER | text_alignment::VCONFORM, true);
    ui::Write(description.GetChar());
    ui::VSpace(5);

    StringBuilder sb("Base stats: ");
    int stat_num = 0;
    for (int i=0; i < ship_stats::MAX; i++) {
        if (stats[i] > 0) {
            sb.AddFormat("%s %3d  ", ship_stats::icons[i], stats[i]);
            stat_num++;
        }
    }
    if (stat_num > 0) {
        ui::Write(sb.c_str);
    }
}

bool ShipClass::CanUseFuel(resources::T resource) const {
    return available_fuels & (1 << resource);
}

resources::T ShipClass::GetNextAvailableFuelType(resources::T resource) const {
    int i = resource + 1;
    for (;!(available_fuels & (1 << i)); i++) {
        if (i >= resources::MAX) {
            return resources::NONE;
        }
    }
    return (resources::T)i;
}

int ShipClass::AvailableFuelTypeCount() const {
    // 'Hamming weight'
    std::bitset<64> fuels_bitset(available_fuels);
    return fuels_bitset.count();
}

Ship::Ship() {
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }
    production_process = 0;
    production_queue.Clear();
}

Ship::~Ship() {}

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
    HandleButtonSound(button_state_flags::JUST_PRESSED);
}

void Ship::_OnNewPlanClicked() {
    TransferPlanUI& tp_ui = GetGlobalState()->active_transfer_plan;
    if (tp_ui.IsActive() || IsIdValid(tp_ui.ship)) {
        return;
    }
    if (!IsLeading()) return;

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
    if (plan_edit_index == 0 || prepared_plans_count == 0) {
        prepared_plans[plan_edit_index].departure_planet = parent_obj;
        min_time = GlobalGetNow();
        plan_edit_index = 0;
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
    data->SerializeBuffer("damage_taken", damage_taken, ship_variables::names, ship_variables::MAX);
    
    data->SetI("production_process", production_process);
    data->CreateArray("production_process", production_queue.size);
    for(int i=0; i < production_queue.size; i++) {
        data->InsertIntoArrayI("production_process", i, production_queue[i].AsInt());
    }

    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) {
            data->AppendToArray("modules", TextFormat("%d %s%", i, GetModule(modules[i])->id));
        }
    }

    data->CreatChildArray("prepared_plans", prepared_plans_count);
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i].Serialize(data->InsertIntoChildArray("prepared_plans", i));
    }
    transferplan_cycle.Serialize(data);
}

void Ship::Deserialize(const DataNode* data) {
    strcpy(name, data->Get("name", "UNNAMED"));
    allegiance = data->GetI("allegiance", allegiance);
    plan_edit_index = -1;
    ship_class = GetGlobalState()->GetFromStringIdentifier(data->Get("class_id"));
    if (!IsIdValid(ship_class)) {
        FAIL("Invalid ship class")  // TODO fail more gracefully
    }

    data->DeserializeBuffer("damage_taken", damage_taken, ship_variables::names, ship_variables::MAX);
    
    production_process = data->GetI("ship_production_process", 0, true);

    int p_size = data->GetArrayLen("production_process");
    production_queue.Resize(p_size);
    production_queue.size = p_size;
    for(int i=0; i < production_queue.size; i++) {
        production_queue.buffer[i] = RID(data->GetArrayElemI("production_process", i));
    }

    data->DeserializeBuffer("transporting", transporting, resources::names, resources::MAX);

    // Fill ship modules
    int modules_count = data->GetArrayLen("modules", true);
    if (modules_count > SHIP_MAX_MODULES) modules_count = SHIP_MAX_MODULES;
    const ShipModules* sms = GetShipModules();
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        modules[i] = GetInvalidId();
    }
    for(int i=0; i < modules_count; i++) {
        const char* identifier_package = data->GetArrayElem("modules", i);

        int index;
        RID module_rid = DeserializeModuleInfo(identifier_package, &index);
        if (!IsIdValid(module_rid)) continue;
        
        module_types::T module_type = sms->GetModuleByRID(module_rid)->type;
        int proper_index = GetFreeModuleSlot(module_type).index;
        if (index < 0) {
            // Unspecified index
            index = proper_index;
        } else if (IsIdValidTyped(modules[index], EntityType::MODULE_CLASS)) {
            // Index not available
            WARNING("Ship already has module at %d", index)
            index = proper_index;
        }

        modules[index] = module_rid;
    }

    // Deserialize transfer plans
    prepared_plans_count = data->GetChildArrayLen("prepared_plans", true);
    if (prepared_plans_count > SHIP_MAX_PREPARED_PLANS) prepared_plans_count = SHIP_MAX_PREPARED_PLANS;
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i] = TransferPlan();
        prepared_plans[i].Deserialize(data->GetChildArrayElem("prepared_plans", i, true));
    }
    
    transferplan_cycle.Deserialize(data);
}

double Ship::GetOperationalMass() const {  // Without resources
    double res = 0.0;

    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i]) && modules[i] != GetShipModules()->expected_modules.droptank_water) {
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
    int drop_tanks = CountWorkingDroptanks();
    double capacity = sc->GetPayloadCapacityMass(dv, drop_tanks);
    return KGToResourceCounts(capacity - GetOperationalMass());
}

resource_count_t Ship::GetFuelRequiredFull(double dv) const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountWorkingDroptanks();
    const resource_count_t fuel_per_droptank = 10;
    double extra_fuel = fuel_per_droptank * drop_tanks;
    return sc->max_capacity + extra_fuel - KGToResourceCounts(sc->GetPayloadCapacityMass(dv, extra_fuel));
}

resource_count_t Ship::GetFuelRequired(double dv, resource_count_t payload) const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountWorkingDroptanks();
    const resource_count_t fuel_per_droptank = 10;
    double extra_fuel = fuel_per_droptank * drop_tanks;
    double dry_mass = sc->oem + GetOperationalMass() + ResourceCountsToKG(payload);
    return KGToResourceCounts((exp(dv/sc->v_e) - 1) * dry_mass - extra_fuel);
}

double Ship::GetCapableDV() const {
    const ShipClass* sc = GetShipClassByRID(ship_class);
    int drop_tanks = CountWorkingDroptanks();

    const resource_count_t fuel_per_droptank = 10;
    
    return sc->v_e * log((ResourceCountsToKG(sc->max_capacity + fuel_per_droptank * drop_tanks) + sc->oem) / (GetOperationalMass() + sc->oem));

    // max_dv = v_e * log(max_capacity / oem + 1)
    // exp(max_dv / v_e) - 1

    // oem = max_capacity / (exp(max_dv/v_e) - 1);
    // sc->v_e * log(max_capacity / oem + 1);
}

bool Ship::IsPlayerControlled() const {
    return allegiance == 0;
}

intel_level::T Ship::GetIntelLevel() const {
    return intel_level::FULL;

    if (IsPlayerControlled()) {
        return intel_level::FULL;
    }
    return intel_level::STATS | (is_detected * intel_level::TRAJECTORY);
}

bool Ship::IsTrajectoryKnown(int index) const {
    return true;
    if (IsPlayerControlled()) {
        return true;
    }
    if((GetIntelLevel() & intel_level::TRAJECTORY) == 0) {
        return false;
    }
    return index == 0 && !IsParked();
}

int Ship::GetCombatStrength() const {
    // Expected damage dealt + expected damage absorbed
    // = offensive * ttk + defensive * ttk + initiative*emeny_attack
    // = offensive * ttk + defensive * ttk + initiative*emeny_attack
    //  | Assuming enemy deals 1 attack in both ordnance and kinetic
    // = (offensive + defensive) * (enemy_hp/offensive + initiative)
    // = (offensive² + defensive * offensive)/ + (offensive + defensive)*initiative

    int offensive = kinetic_offense() + ordnance_offense();
    int defensive = kinetic_defense() + ordnance_defense();
    int hp = kinetic_hp() - damage_taken[ship_variables::HP];
    float effective_hp = hp * (1 + defensive / 3.0f + initiative() / 3.0f);
    return (int) (offensive * effective_hp);
}

int Ship::GetMissingHealth() const {
    //SHOW_I(stats[ship_stats::KINETIC_HP])
    //SHOW_I(stats[ship_stats::ENERGY_HP])
    //SHOW_I(stats[ship_stats::CREW])
    //SHOW_I(variables[ship_variables::KINETIC_ARMOR])
    //SHOW_I(variables[ship_variables::ENERGY_ARMOR])
    //SHOW_I(variables[ship_variables::CREW])
    
    return
          damage_taken[ship_variables::HP]
        //+ damage_taken[ship_variables::ENERGY_ARMOR]
        + damage_taken[ship_variables::CREW]
    ;
}

const char* _GetTypeIcon(ship_type::T ship_type) {
    switch (ship_type) {
        case ship_type::SHIPYARD: return " " ICON_STATION;
        case ship_type::UTILITY: return " " ICON_UTIL_SHIP;
        case ship_type::TRANSPORT: return " " ICON_TRANSPORT_SHIP;
        case ship_type::MILITARY: return " " ICON_MIL_SHIP;
        default: return ICON_EMPTY;
    }
}

const char *Ship::GetTypeIcon() const {
    return _GetTypeIcon(GetShipType());
}

bool Ship::CanDragModule(int index) const {
    if (!IsIdValid(modules[index]))
        return true;
    if (GetModule(modules[index])->delta_stats[ship_stats::KINETIC_HP] > kinetic_hp() - damage_taken[ship_variables::HP]) {
        return false;
    }
    //if (GetModule(modules[index])->delta_stats[ship_stats::ENERGY_HP] > energy_hp() - damage_taken[ship_variables::ENERGY_ARMOR]) {
    //    return false;
    //}
    if (GetModule(modules[index])->delta_stats[ship_stats::CREW] > crew() - damage_taken[ship_variables::CREW]) {
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

bool Ship::CanProduce() const {
    for (int i = ship_stats::INDUSTRIAL_ADMIN; i < ship_stats::MAX; i++) {
        if (stats[i]) return true;
    }
    return false;
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

int Ship::CountWorkingDroptanks() const {
    int res = 0;
    const ShipClass* sc = GetShipClassByRID(ship_class);
    resources::T rsc = sc->GetNextAvailableFuelType(resources::NONE);
    for(;rsc != resources::NONE; rsc = sc->GetNextAvailableFuelType(rsc))  // Iterates over fuel types
    for(int i=0; i < SHIP_MAX_MODULES; i++)  // Iterates over modules
        if (GetShipModules()->IsDropTank(modules[i], rsc)) 
            res++;
    return res;
}

ship_type::T Ship::GetShipType() const {
    if (IsStatic()) return ship_type::SHIPYARD;
    for(int i=0; i < SHIP_MAX_MODULES; i++) {
        //if (GetShipModules()->GetModuleByRID(modules[i])->production) return ShipType::UTILITY;
        // TODO: when is it 'UTILITY'?
    }
    if (GetCombatStrength() > 0) return ship_type::MILITARY;
    return ship_type::TRANSPORT;
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
    _UpdateTransferCycle();
}

void Ship::_UpdateShipyard() {
    int repair_stations = MinInt(industrial_dock(), industrial_manufacturing());
    if (GetCalendar()->IsNewDay() && IsParked()) {
        for(int i=0; i < repair_stations; i++) {
            IDList available_ships;
            ship_selection_flags::T selection_flags = ship_selection_flags::GetAllegianceFlags(allegiance);
            GetShips()->GetOnPlanet(&available_ships, parent_obj, selection_flags);
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
        AdvanceProductionQueue();
    }
}

void Ship::_UpdateModules() {
    // Recalculate every frame

    // Gotta exist a more efficient way
    // And robust. Dependency graph (also a way to find circular dependencies)


    for (int i=0; i < ship_stats::MAX; i++) {
        stats[i] = GetShipClassByRID(ship_class)->stats[i];
    }

    List<int> uncertain_module_indices = List<int>(SHIP_MAX_MODULES);  // Could be put on the stack, since we know the max capacity

    // Collect modules that need to be re-updated
    for (int mod_idx=0; mod_idx < SHIP_MAX_MODULES; mod_idx++) {
        modules_enabled[mod_idx] = false;
        if (!IsIdValid(modules[mod_idx])) continue;

        const ShipModuleClass* smc = GetModule(modules[mod_idx]);
        if (smc->HasStatDependencies()) {
            uncertain_module_indices.Append(mod_idx);
        } else {
            modules_enabled[mod_idx] = true;
            for (int stat_idx=0; stat_idx < (int) ship_stats::MAX; stat_idx++) {
                stats[stat_idx] += smc->delta_stats[stat_idx];
            }
        }
    }

    // Over 5 iterations ...
    for (int iter = 0; iter < 5 && uncertain_module_indices.Count() > 0; iter++) {
        // Lock in modules that can be activated
        for (int i = uncertain_module_indices.Count() - 1; i >= 0; i--) {
            int mod_idx = uncertain_module_indices[i];

            const ShipModuleClass* smc = GetModule(modules[mod_idx]);
            bool is_activated = true;
            for (int stat_idx=0; stat_idx < (int) ship_stats::MAX; stat_idx++) {
                if (stats[stat_idx] < smc->required_stats[stat_idx]) {
                    is_activated = false;
                }
            }
            if (is_activated) {
                uncertain_module_indices.EraseAt(i);
                modules_enabled[mod_idx] = true;
                for (int stat_idx=0; stat_idx < (int) ship_stats::MAX; stat_idx++) {
                    stats[stat_idx] += smc->delta_stats[stat_idx];
                }
            }
        }
    }

    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        if (IsIdValid(modules[i])) {
            GetModule(modules[i])->UpdateCustom(this);
        }
    }
}

void Ship::_UpdateTransferCycle() {
    while (prepared_plans_count < transferplan_cycle.stops) {
        // Figure out where we are in the cycle
        int cycle_index = -1;
        for (int i=0; i < transferplan_cycle.stops; i++) {
            bool cycle_match = true;
            for (int j=0; j < prepared_plans_count; j++) {
                const TransferPlan* last_tp = &prepared_plans[prepared_plans_count - 1 - j];
                int cycle_check_index = (i - j + transferplan_cycle.stops) % transferplan_cycle.stops;
                if (last_tp->arrival_planet != transferplan_cycle.planets[cycle_check_index]) {
                    cycle_match = false;
                    break;
                }
            }
            if (cycle_match) {
                cycle_index = (i + 1) % transferplan_cycle.stops;  // Next one
                break;
            }
        }
        if (cycle_index < 0) {
            ERROR("Ship broke cycle: cannot complete")
            return;
        }
        
        prepared_plans[prepared_plans_count] = TransferPlan();
        // Create new transfer plan
        TransferPlan* tp = &prepared_plans[prepared_plans_count];
        if (prepared_plans_count > 0) {
            tp->departure_planet = prepared_plans[prepared_plans_count - 1].arrival_planet;
        } else {
            tp->departure_planet = GetParentPlanet();
        }
        tp->arrival_planet = transferplan_cycle.planets[cycle_index];

        ASSERT(tp->arrival_planet != tp->departure_planet)

        TransferPlanSoonest(tp, transferplan_cycle.dvs[cycle_index], prepared_plans[prepared_plans_count - 1].arrival_time);
        TransferPlanSolve(tp);
        
        for (int resource_idx = 0; resource_idx < resources::MAX; resource_idx++) {
            tp->resource_transfer[resource_idx] = transferplan_cycle.resource_transfers[cycle_index][resource_idx];
        }
        prepared_plans_count++;
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
    
    Icon3D* icon_inst = GetRenderServer()->icons.GetOrAllocate(&icon3d);
    icon_inst->scale = icon_size;
    icon_inst->offset = draw_offset;
    icon_inst->atlas_pos = AtlasPos(type, 28);
    icon_inst->color = GetColor();
    icon_inst->world_pos = position.cartesian;
    Vector3 from = GameCamera::WorldToMacro(position.cartesian);
    Vector3 to = icon_inst->GetFinalRenderPos();

    if (GetCamera()->IsInView(from) && GetCamera()->IsInView(to)) {
        DrawLine3D(from, to, GetColor());
    }
    
    Text3D* text_inst = GetRenderServer()->text_labels_3d.GetOrAllocate(&text3d);
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
    text_inst->alignment = (type < 2 ? text_alignment::RIGHT : text_alignment::LEFT) | text_alignment::VCENTER;

    //DrawLineV(draw_pos, Vector2Add(draw_pos, draw_offset), GetColor());
    
    //DrawRectangleV(Vector2SubtractValue(draw_pos, 4.0f), {8, 8}, color);
}

void Ship::DrawTrajectories() const {
    Color color = GetColor();
    
    if ((GetIntelLevel() & intel_level::TRAJECTORY) == 0) {
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
        //RenderOrbit(&tf_orbit, i == plan_edit_index ? Palette::ui_main : GetColor());
        //OrbitSegment tf_orbit = OrbitSegment(&plan->transfer_orbit[plan->primary_solution]);

        ConicRenderInfo cri;
        if (i == 0) {
            cri = ConicRenderInfo::FromOrbitSegment(&tf_orbit, GlobalGetNow(), DVector3::Zero(), orbit_render_mode::Solid, color);
        } else if (mouse_hover || GetGlobalState()->focused_ship == id) {
            cri = ConicRenderInfo::FromOrbitSegment(&tf_orbit, GlobalGetNow(), DVector3::Zero(), orbit_render_mode::Solid, ColorBrightness(color, -0.5));
        }
        GetRenderServer()->QueueConicDraw(cri);
    }
}

void Ship::Inspect() {

}

void Ship::RemoveShipModuleAt(int index) {
    modules[index] = GetInvalidId();
}

void Ship::Repair(int hp) {
    if (hp > stats[ship_stats::KINETIC_HP] + stats[ship_stats::CREW]){
        // for initialisation
        damage_taken[ship_variables::HP] = 0;
        //damage_taken[ship_variables::ENERGY_ARMOR] = 0;
        damage_taken[ship_variables::CREW] = 0;
        return;
    }
    int kinetic_repair = MinInt(hp, damage_taken[ship_variables::HP]);
    hp -= kinetic_repair;
    //int energy_repair = MinInt(hp, damage_taken[ship_variables::ENERGY_ARMOR]);
    //hp -= energy_repair;
    int crew_repair = MinInt(hp, damage_taken[ship_variables::CREW]);
    damage_taken[ship_variables::HP] -= kinetic_repair;
    //damage_taken[ship_variables::ENERGY_ARMOR] -= energy_repair;
    damage_taken[ship_variables::CREW] -= crew_repair;
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

bool Ship::CanProduce(RID object, bool check_resources, bool check_stats) const  {
    if (!IsParked()) return false;
    if (!IsIdValid(object)) return false;
    const resource_count_t* construction_resources = NULL;
    const int* construction_required = NULL;
    switch (IdGetType(object)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByRID(object);
            construction_resources = ship_class->construction_resources;
            construction_required = ship_class->construction_requirements;
            break;
        }
        case EntityType::MODULE_CLASS: {
            const ShipModuleClass* module_class = GetModule(object);
            construction_resources = module_class->construction_resources;
            construction_required = module_class->construction_requirements;
            break;
        }
        default: break;
    }

    if (check_resources && construction_resources == NULL) return false;
    if (check_stats && construction_required == NULL) return false;
    if (check_resources) {
        const PlanetaryEconomy* economy = &GetPlanet(GetParentPlanet())->economy;
        for (int i=0; i < resources::MAX; i++) {
            if (construction_resources[i] > economy->resource_stock[i]) {
                return false;
            }
        }
    }
    if (check_stats) {
        for (int i=0; i < ship_stats::MAX; i++) {
            if (construction_required[i] > stats[i]) {
                return false;
            }
        }
    }
    return true;
}

void Ship::AdvanceProductionQueue() {
    if (!IsParked()) return;
    if (production_queue.size == 0) return;
    RID candidate = GetInvalidId();
    for (int i=0; i < production_queue.size; i++) {
        if (CanProduce(production_queue.Get(i), true, true)) {
            candidate = production_queue.Get(i);
            break;
        }
    }
    
    if (candidate == GetInvalidId()) {
        // TODO: notify the player
        RID current_id = production_queue.Get(0);
        if (IsIdValidTyped(current_id, EntityType::SHIP_CLASS)) {
            ERROR("CANNOT PRODUCE '%s' ON '%s'", GetShipClassByRID(production_queue.Get(0))->name.GetChar(), name)
        } else if (IsIdValidTyped(current_id, EntityType::MODULE_CLASS)) {
            ERROR("CANNOT PRODUCE '%s' ON '%s'", GetModule(production_queue.Get(0))->name.GetChar(), name)
        } else {
            ERROR("INVALID PRODUCTION ITEM ON '%s'", name)
        }
        return;
    }
    production_process++;

    Planet* planet = GetPlanet(GetParentPlanet());
    const resource_count_t* construction_resources = NULL;

    // Handle spawning in, if applicable (early return in case it's not ready)
    RID product_id = production_queue.Get(0);
    if (IsIdValidTyped(product_id, EntityType::MODULE_CLASS)) {
        const ShipModuleClass* smc = GetModule(product_id);
        if (production_process < smc->GetConstructionTime()) return;
        
        ShipModuleSlot free_slot = GetFreeModuleSlot(smc->type);
        if (!free_slot.IsValid()) {
            free_slot = planet->GetFreeModuleSlot(smc->type);
        }
        if (free_slot.IsValid()) {
            free_slot.SetSlot(candidate);
        }
        construction_resources = smc->construction_resources;
    }
    else if (IsIdValidTyped(product_id, EntityType::SHIP_CLASS)) {
        const ShipClass* sc = GetShipClassByRID(product_id);
        if (production_process < sc->construction_time) return;

        IDList list;
        GetShips()->GetOnPlanet(&list, id, ship_selection_flags::ALL);

        DataNode ship_data;
        StringBuilder sb;
        GetShips()->GetRandomShipName(product_id, &sb);
        ship_data.Set("name", sb.c_str);
        ship_data.Set("class_id", sc->id);
        ship_data.SetI("allegiance", allegiance);        ship_data.Set("planet", planet->name.GetChar());
        ship_data.CreateArray("tf_plans", 0);
        ship_data.CreateArray("modules", 0);
        GetShips()->AddShip(&ship_data);
        construction_resources = sc->construction_resources;
    }
    GetTechTree()->ReportProduction(product_id);

    if (construction_resources != NULL) {
        for (int i=0; i < resources::MAX; i++) {
            if (construction_resources[i] != 0) {
                planet->economy.TakeResource((resources::T) i, construction_resources[i]);
            }
        }
    }

    production_queue.EraseAt(0);
    production_process = 0;
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

    resource_count_t fuel_quantity = local_economy->TakeResource(tp->fuel_type, tp->fuel);
    if (fuel_quantity < tp->fuel && IsPlayerControlled()) {
        resource_count_t remaining_fuel = tp->fuel - fuel_quantity;
        // Abort
        USER_INFO(
            "Not enough fuel. Could not afford/access remaining fuel %d cts on %s", 
            remaining_fuel, GetPlanet(tp->departure_planet)->name.GetChar()
        )
        local_economy->GiveResource(tp->fuel_type, fuel_quantity);
        prepared_plans_count = 0;
        return;
    }

    for (int i=0; i < resources::MAX; i++) {
        transporting[i] = local_economy->TakeResource((resources::T) i, tp->resource_transfer[i]);
    }
    //INFO("%s: %d", resources::names[transporing.resource_id], transporing.quantity)

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
    ship_selection_flags::T selection_flags = (~ship_selection_flags::GetAllegianceFlags(leading_ship->allegiance) & 0xFF) | ship_selection_flags::MILITARY;
    GetShips()->GetOnPlanet(&hostile_ships, tp->arrival_planet, selection_flags);
    IDList allied_ships;
    GetShips()->GetFleet(&allied_ships, leading_ship->id);
    allied_ships.Append(leading_ship->id);
    if (hostile_ships.size > 0) {
        // First, military ships vs. military ships
        bool victory = ShipBattle(&allied_ships, &hostile_ships, tp->arrival_dvs[tp->primary_solution].Length());
        // allied ship and hostile ship might contain dead ships
        if (victory) {
            ship_selection_flags::T selection_flags = (~ship_selection_flags::GetAllegianceFlags(leading_ship->allegiance) & 0xFF) | ship_selection_flags::ALL_SELECTION;
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
        ship_selection_flags::T selection_flags = (~ship_selection_flags::GetAllegianceFlags(leading_ship->allegiance) & 0xFF) | ship_selection_flags::ALL_SELECTION;
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

    // Drop off resources
    int total_resources = 0;
    for (int i=0; i < resources::MAX; i++) {
        GetPlanet(tp->arrival_planet)->economy.GiveResource((resources::T) i, transporting[i]);
        total_resources += transporting[i];
    }

    // Report visit
    StringBuilder var_name_sb;
    var_name_sb.Add("visit_").AddPerma(GetPlanet(GetParentPlanet())->name);
    global_vars::Inc(var_name_sb.c_str, 1);
    global_vars::Inc("acc_delivered_counts", total_resources);

    position = GetPlanet(GetParentPlanet())->position;

    // Remove drop tanks
    const ShipClass* sc = GetShipClassByRID(ship_class);
    resources::T rsc = sc->GetNextAvailableFuelType(resources::NONE);
    for(;rsc != resources::NONE; rsc = sc->GetNextAvailableFuelType(rsc))  // Iterates over fuel types
    for(int i=0; i < SHIP_MAX_MODULES; i++)  // Iterates over modules
        if (GetShipModules()->IsDropTank(modules[i], rsc)) {
            RemoveShipModuleAt(i);
            global_vars::Inc("droptanks_used", 1);
        }

    RemoveTransferPlan(0);

    // TODO: update Transfer plans according to cycle

    Update();
}

bool ship_selection_flags::MatchesShip(T selection_flags, const Ship *ship) {
    if (((1 << ship->allegiance) & selection_flags) == 0)  return false;
    bool is_mil = ship->kinetic_offense() + ship->ordnance_offense() > 0;
    if (!(selection_flags & ship_selection_flags::MILITARY) && is_mil)  return false;
    if (!(selection_flags & ship_selection_flags::CIVILIAN) && !is_mil)  return false;
    return true;
}

Ships::Ships() {
    alloc.Init();
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
    name_library = *data->GetChild("ship_names");

    for (int index=0; index < ship_classes_count; index++) {
        const DataNode* sc_data = data->GetChildArrayElem("ship_classes", index);
        ShipClass sc;

        sc.name = PermaString(sc_data->Get("name", "[NAME MISSING]"));
        sc.description = PermaString(sc_data->Get("description", "[DESCRITION MISSING]"));
        strcpy(sc.id, sc_data->Get("id", "_"));

        sc.max_capacity = sc_data->GetI("capacity", 0);
        sc.max_dv = sc_data->GetF("dv", 0) * 1000;  // km/s -> m/s
        sc.v_e = sc_data->GetF("Isp", 0) * 1000;    // km/s -> m/s
        sc.available_fuels = 0;
        for(int i=0; i < sc_data->GetArrayLen("fuel", true); i++) {
            sc.available_fuels |= 1 << FindResource(sc_data->GetArrayElem("fuel", i, "", true), resources::WATER);
        }
        sc.construction_time = sc_data->GetI("construction_time", 20);
        sc.oem = ResourceCountsToKG(sc.max_capacity) / (exp(sc.max_dv/sc.v_e) - 1);
        sc.construction_batch_size = sc_data->GetI("batch_size", 1, true);
        sc.is_hidden = strcmp(sc_data->Get("hidden", "n", true), "y") == 0;
        sc.icon_index = AtlasPos(
            sc_data->GetArrayElemI("icon_index", 0, 31),
            sc_data->GetArrayElemI("icon_index", 1, 31));
        sc.module_config.Load(module_configurations, sc.id);
        strcpy(sc.naming_convention, sc_data->Get("naming_convention", "cargo"));

        if (sc_data->HasChild("construction_requirements")) {
            sc_data->DeserializeBuffer("construction_requirements", sc.construction_requirements, ship_stats::names, ship_stats::MAX);
        } else {  // Set default requirements
            for (int j=0; j < ship_stats::MAX; j++) {
                sc.construction_requirements[j] = 0;
            }
            sc.construction_requirements[ship_stats::INDUSTRIAL_ADMIN] = 1;
            sc.construction_requirements[ship_stats::INDUSTRIAL_STORAGE] = 1;
            sc.construction_requirements[ship_stats::INDUSTRIAL_MANUFACTURING] = 1;
            sc.construction_requirements[ship_stats::INDUSTRIAL_DOCK] = 1;
        }

        sc_data->DeserializeBuffer("construction_resources", sc.construction_resources, resources::names, resources::MAX);
        sc_data->DeserializeBuffer("stats", sc.stats, ship_stats::names, ship_stats::MAX);

        //ASSERT_ALOMST_EQUAL_FLOAT(sc.v_e * log((ResourceCountsToKG(sc.max_capacity) + sc.oem) / sc.oem), sc.max_dv)   // Remove when we're sure thisworks

        ship_classes[index] = sc;
        RID rid = RID(index, EntityType::SHIP_CLASS);

        GetGlobalState()->AddStringIdentifier(ship_classes[index].id, rid);
    }
    return ship_classes_count;
}

Ship* Ships::GetShip(RID id) const {
    if ((!alloc.ContainsID(id))) {
        FAIL("Invalid id (%d)", id)
    }
    return (Ship*) alloc.Get(id);
}

void Ships::GetOnPlanet(IDList* list, RID planet, ship_selection_flags::T selection) const {
    for (auto it = alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        if (ship->IsParked() && planet != ship->GetParentPlanet()) 
            continue;
        if (!ship->IsParked() && planet != GetInvalidId()) 
            continue;
        if (ship_selection_flags::MatchesShip(selection, ship)) {
            list->Append(it.GetId());
        }
    }
}

void Ships::GetAll(IDList *list, ship_selection_flags::T selection) const {
    for (auto it = alloc.GetIter(); it; it++) {
        const Ship* ship = GetShip(it.GetId());
        if (ship_selection_flags::MatchesShip(selection, ship)) {
            list->Append(it.GetId());
        }
    }
}

void Ships::GetFleet(IDList *list, RID leading) const {
    for (auto it = alloc.GetIter(); it; it++) {
        const Ship* ship = GetShip(it.GetId());
        if (ship->parent_obj == leading) {
            list->Append(it.GetId());
        }
    }
}

void Ships::KillShip(RID uuid, bool notify_callback) {
    if (!IsIdValidTyped(uuid, EntityType::SHIP)) {
        return;
    }
    GetRenderServer()->icons.EraseAt(GetShip(uuid)->icon3d);  // Destroy it here, since GetRenderServer() may not exist in destructor
    GetRenderServer()->text_labels_3d.EraseAt(GetShip(uuid)->text3d);
    alloc.EraseAt(uuid);  // Calls destructor
}

void Ships::DrawShipClassUI(RID uuid) const {
    if(!IsIdValidTyped(uuid, EntityType::SHIP_CLASS)) {  // empty
        return;
    } else {  // filled
        const ShipClass* sc = GetShipClassByRID(uuid);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::PushMouseHint(GetMousePosition(), 400, 400);
            ui::Current()->flexible = true;
            sc->MouseHintWrite();
            ui::Current()->extend_x = 400;  // Force width
            ui::EncloseDynamic(0, Palette::bg, Palette::ui_main, 4);
            ui::Pop();
        }
        ui::DrawIcon(sc->icon_index, text_alignment::CENTER, Palette::ui_main, 40);
    }
}

void Ships::GetRandomShipName(RID ship_class_id, StringBuilder* sb) const {
    const ShipClass* ship_class = GetShipClassByRID(ship_class_id);
    const char* key = ship_class->naming_convention;
    int choice_domain_size = name_library.GetArrayLen(key);
    if (choice_domain_size == 0) {
        ERROR("No such naming convention '%s'", key)
        sb->Add("[INVALID NAMING CONVENTION]");
        return;
    }
    int choice = rand() % choice_domain_size;
    sb->Add(name_library.GetArrayElem(key, choice));
}

void Ships::Clear() {
    for (auto it = alloc.GetIter(); it; it++) {
        // Make sure icons are destroyed etc.
        KillShip(it.GetId(), false);
    }
    alloc.Clear();
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
