#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"

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
    HandleButtonSound(BUTTON_STATE_FLAG_JUST_PRESSED);
}

void Ship::_OnNewPlanClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (tp_ui.IsActive() || IsIdValid(tp_ui.ship)) {
        return;
    }

    GetCalendar()->paused = true;

    // Append new plan

    ASSERT(prepared_plans_count != 0 || is_parked)
    if (prepared_plans_count >= SHIP_MAX_PREPARED_PLANS) {
        ERROR("Maximum transfer plan stack reached (ship %s)", name);
        return;
    }

    prepared_plans[prepared_plans_count] = TransferPlan();
    plan_edit_index = prepared_plans_count;

    timemath::Time min_time = 0;
    if (plan_edit_index == 0) {
        prepared_plans[plan_edit_index].departure_planet = parent_planet;
        min_time = GlobalGetNow();
    } else {
        prepared_plans[plan_edit_index].departure_planet = prepared_plans[plan_edit_index - 1].arrival_planet;
        min_time = prepared_plans[plan_edit_index - 1].arrival_time;
    }

    tp_ui.Make();
    tp_ui.SetPlan(&prepared_plans[plan_edit_index], id, min_time, 1e20);
    prepared_plans_count++;
}

void Ship::CreateFrom(const ShipClass *sc) {
    
}

void Ship::Serialize(DataNode *data) const {
    data->Set("name", name);
    data->Set("is_parked", is_parked ? "y" : "n");
    
    data->Set("class_id", GetShipClassByIndex(ship_class)->id);
    data->SetI("resource_qtt", transporing.quantity);
    data->SetI("resource_id", transporing.resource_id);

    data->SetArray("modules", modules_count);
    for(int i=0; i < modules_count; i++) {
        data->SetArrayElem("modules", i, GetModuleByIndex(modules[i])->id);
    }

    data->SetArrayChild("prepared_plans", prepared_plans_count);
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i].Serialize(data->SetArrayElemChild("prepared_plans", i, DataNode()));
    }
}

void Ship::Deserialize(const DataNode* data) {
    strcpy(name, data->Get("name", "UNNAMED"));
    is_parked = strcmp(data->Get("is_parked", "y", true), "y") == 0;
    plan_edit_index = -1;
    ship_class = GlobalGetState()->ships.GetShipClassIndexById(data->Get("class_id"));
    transporing.quantity = data->GetI("resource_qtt", 0);
    transporing.resource_id = (ResourceType) data->GetI("resource_id", RESOURCE_NONE);

    color = Palette::green;


    modules_count = data->GetArrayLen("modules", true);
    if (modules_count > SHIP_MAX_MODULES) modules_count = SHIP_MAX_MODULES;
    for(int i=0; i < modules_count; i++) {
        modules[i] = GetModuleIndexById(data->GetArray("modules", i));
    }

    prepared_plans_count = data->GetArrayChildLen("prepared_plans", true);
    if (prepared_plans_count > SHIP_MAX_PREPARED_PLANS) prepared_plans_count = SHIP_MAX_PREPARED_PLANS;
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i] = TransferPlan();
        prepared_plans[i].Deserialize(data->GetArrayChild("prepared_plans", i, true));
    }
}

double Ship::GetPayloadMass() const{
    double res = 0.0;

    QuestManager* qm = &GlobalGetState()->quest_manager;
    for(auto i = qm->active_quests.GetIter(); i; i++) {
        if (qm->active_quests[i]->ship == id) {
            res += qm->active_quests[i]->payload_mass;
        }
    }

    for(int i=0; i < modules_count; i++) {
        res += GetModuleByIndex(modules[i])->mass;
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

TransferPlan* Ship::GetEditedTransferPlan() {
    if (plan_edit_index < 0) return NULL;
    return &prepared_plans[plan_edit_index];
}

void Ship::ConfirmEditedTransferPlan() {
    ASSERT(prepared_plans_count != 0 || is_parked)
    ASSERT(!is_parked || IsIdValid(parent_planet))
    TransferPlan& tp = prepared_plans[prepared_plans_count - 1];
    if (prepared_plans_count == 1 && parent_planet != tp.departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at current planet)", name);
        return;
    }
    else if (prepared_plans_count > 1 && prepared_plans[prepared_plans_count - 2].arrival_planet != tp.departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at planet last visited)", name);
        return;
    }
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
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
        if (is_parked) {
            prepared_plans[index].departure_planet = parent_planet;
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

    if (prepared_plans_count == 0 || (plan_edit_index == 0 && prepared_plans_count == 1)) {
        position = GetPlanet(parent_planet)->position;
    } else {
        const TransferPlan& tp = prepared_plans[0];
        if (is_parked) {
            if (tp.departure_time < now) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(parent_planet)->position;
            }
        } else {
            if (tp.arrival_time < now) {
                _OnArrival(tp);
            } else {
                position = tp.transfer_orbit[tp.primary_solution].GetPosition(now);
            }
        }
    }

    draw_pos = GetScreenTransform()->TransformV(position.cartesian);
    if (is_parked) {
        double rad = fmax(GetScreenTransform()->TransformS(GetPlanet(parent_planet)->radius), 4) + 8.0;
        double phase = 20.0 /  rad * index_on_planet;
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
    }

    for (int i=0; i < modules_count; i++) {
        GetModuleByIndex(modules[i])->Update(this);
    }
}

void _DrawShipAt(Vector2 pos, Color color) {
    DrawRectangleV(Vector2SubtractValue(pos, 4.0f), {8, 8}, color);
}

void Ship::Draw(const CoordinateTransform* c_transf) const {
    _DrawShipAt(draw_pos, color);

    for (int i=0; i < prepared_plans_count; i++) {
        if (i == plan_edit_index) {
            continue;
        }
        const TransferPlan& plan = prepared_plans[i];
        OrbitPos to_departure = plan.transfer_orbit[plan.primary_solution].GetPosition(
            timemath::Time::Latest(plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = plan.transfer_orbit[plan.primary_solution].GetPosition( 
            plan.arrival_time
        );
        plan.transfer_orbit[plan.primary_solution].DrawBounded(to_departure, to_arrival, 0, 
            ColorAlpha(color, i == highlighted_plan_index ? 1 : 0.5)
        );
        if (i == plan_edit_index){
            plan.transfer_orbit[plan.primary_solution].DrawBounded(to_departure, to_arrival, 0, Palette::ui_main);
        }
    }
    if (plan_edit_index >= 0 && IsIdValid(prepared_plans[plan_edit_index].arrival_planet)) {
        const TransferPlan& last_tp = prepared_plans[plan_edit_index];
        OrbitPos last_pos = GetPlanet(last_tp.arrival_planet)->orbit.GetPosition(
            last_tp.arrival_time
        );
        Vector2 last_draw_pos = c_transf->TransformV(last_pos.cartesian);
        _DrawShipAt(last_draw_pos, ColorAlpha(color, 0.5));
    }
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
            resource_name = GetResourceData(ship->prepared_plans[i].resource_transfer.resource_id).name;
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
        ButtonStateFlags button_state = UIContextAsButton();
        HandleButtonSound(button_state & BUTTON_STATE_FLAG_JUST_PRESSED);
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            ship->highlighted_plan_index = i;
        }
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            ship->StartEditingPlan(i);
        }
        UIContextPop();  // HSplit

        if (i != ship->plan_edit_index) {
            UIContextPushHSplit(-32, -1);
            UIContextWrite("X");
            UIContextEnclose(Palette::bg, Palette::blue);
            ButtonStateFlags button_state = UIContextAsButton();
            HandleButtonSound(button_state & BUTTON_STATE_FLAG_JUST_PRESSED);
            if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
                ship->RemoveTransferPlan(i);
            }
            UIContextPop();  // HSplit
        }

        UIContextPop();  // Insert
    }
    if (UIContextDirectButton("+", 10) & BUTTON_STATE_FLAG_JUST_PRESSED) {
        ship->_OnNewPlanClicked();
    }
}

void _UIDrawQuests(Ship* ship) {
    QuestManager* qm = &GlobalGetState()->quest_manager;
    double max_mass = ResourceCountsToKG(ship->GetMaxCapacity()) - ship->GetPayloadMass();
    
    for(auto it = qm->active_quests.GetIter(); it; it++) {
        Quest* quest = qm->active_quests.Get(it);
        bool is_quest_in_cargo = quest->ship == ship->id;
        if (quest->current_planet != ship->parent_planet && !is_quest_in_cargo) continue;
        bool can_accept = quest->payload_mass <= max_mass;
        ButtonStateFlags button_state = quest->DrawUI(true, is_quest_in_cargo);
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED && can_accept) {
            if (is_quest_in_cargo) {
                qm->PutbackQuest(ship->id, it.index);
            } else {
                qm->PickupQuest(ship->id, it.index);
            }
        }
    }
}

void Ship::DrawUI(const CoordinateTransform* c_transf) {
    if (mouse_hover) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, Palette::red);
        DrawTextEx(GetCustomDefaultFont(), name, Vector2Add(draw_pos, {5, 5}), 16, 1, color);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }

    highlighted_plan_index = -1;
    if (!mouse_hover && GlobalGetState()->active_transfer_plan.ship != id && GlobalGetState()->focused_ship != id) return;

    int text_size = 16;
    UIContextCreate(
        GetScreenWidth() - 20*text_size - 5, 5 + 200,
        20*text_size, GetScreenHeight() - 200 - 2*5, 
        text_size, Palette::ui_main
    );

    UIContextEnclose(Palette::bg, color);

    UIContextWrite(name);
    StringBuilder sb;
    sb.AddFormat("Payload %d / %d ", KGToResourceCounts(GetPayloadMass()), GetMaxCapacity());
    UIContextWrite(sb.c_str);
    UIContextPushInset(0, 3);
    UIContextFillline(KGToResourceCounts(GetPayloadMass()) / GetMaxCapacity(), Palette::ui_main, Palette::bg);
    UIContextPop();  // Inset
    sb.Clear();
    sb.AddFormat("I_sp        %2.2f km/s\n", GetShipClassByIndex(ship_class)->v_e / 1000);
    sb.AddFormat("dv left     %2.2f km/s\n", GetCapableDV());
    UIContextWrite(sb.c_str);

    UIContextPushInset(3, SHIP_MAX_MODULES * 200 / (5*5));
    for (int i=0; i < SHIP_MAX_MODULES; i++) {
        UIContextPushGridCell(5, SHIP_MAX_MODULES / 5, i % 5, i / 5);
        UIContextShrink(3, 3);
        if(i >= modules_count) {
            UIContextEnclose(Palette::bg, GRAY);
            UIContextPop(); // GridCell
            continue;
        }
        UIContextEnclose(Palette::bg, Palette::ui_main);
        if (UIContextAsButton() & BUTTON_STATE_FLAG_HOVER) {
            UISetMouseHint(GetModuleByIndex(modules[i])->name);
        }
        UIContextPop(); // GridCell
    }
    UIContextPop(); // Inset

    _UIDrawTransferplans(this);
    _UIDrawQuests(this);
}

void Ship::Inspect() {
}

void Ship::_OnDeparture(const TransferPlan& tp) {
    PlanetaryEconomy* local_economy = &GetPlanet(tp.departure_planet)->economy;

    ResourceTransfer fuel_tf = local_economy->DrawResource(ResourceTransfer(RESOURCE_WATER, tp.fuel_mass));
    if (fuel_tf.quantity < tp.fuel_mass) {
        resource_count_t remaining_fuel = tp.fuel_mass - fuel_tf.quantity;
        if (local_economy->trading_accessible && local_economy->GetPrice(RESOURCE_WATER, remaining_fuel) < GlobalGetState()->capital) {
            USER_INFO("Automatically purchased %d of water for M§M %ld K", remaining_fuel, local_economy->GetPrice(RESOURCE_WATER, remaining_fuel) / 1e3)
            local_economy->TryPlayerTransaction(ResourceTransfer(RESOURCE_WATER, remaining_fuel));
            local_economy->DrawResource(ResourceTransfer(RESOURCE_WATER, remaining_fuel));
        } else {
            // Abort
            USER_INFO("Not enough fuel. Could not afford/access remaining fuel %d cts on %s", remaining_fuel, GetPlanet(tp.departure_planet)->name)
            local_economy->GiveResource(fuel_tf);
            prepared_plans_count = 0;
            return;
        }
    }

    transporing = local_economy->DrawResource(tp.resource_transfer);

    for(auto i = GlobalGetState()->quest_manager.active_quests.GetIter(); i; i++) {
        if (GlobalGetState()->quest_manager.active_quests[i]->ship == id) {
            GlobalGetState()->quest_manager.QuestDepartedFrom(i.index, parent_planet);
        }
    }

    parent_planet = GetInvalidId();
    is_parked = false;

    Update();
}

void Ship::_OnArrival(const TransferPlan& tp) {
    GetPlanet(tp.arrival_planet)->economy.GiveResource(tp.resource_transfer);
    parent_planet = tp.arrival_planet;
    is_parked = true;
    position = GetPlanet(parent_planet)->position;
    for(auto i = GlobalGetState()->quest_manager.active_quests.GetIter(); i; i++) {
        const Quest* quest = GlobalGetState()->quest_manager.active_quests[i];
        if (quest->ship == id && quest->arrival_planet == tp.arrival_planet) {
            GlobalGetState()->quest_manager.QuestArrivedAt(i.index, tp.arrival_planet);
        }
    }

    RemoveTransferPlan(0);
    Update();
}

void Ship::_EnsureContinuity() {
    INFO("Ensure Continuity Call");
    if (prepared_plans_count == 0) return;
    entity_id_t planet_tracker = parent_planet;
    int start_index = 0;
    if (!is_parked) {
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

    ship_classes_ids = std::map<std::string, shipclass_index_t>();
    ship_classes = NULL;
    ship_classes_count = 0;
}

void Ships::ClearShips(){
    alloc.Clear();
}

entity_id_t Ships::AddShip(const DataNode* data) {
    Ship *ship;
    int ship_entity = alloc.Allocate(&ship);
    
    ship->Deserialize(data);
    if (ship->is_parked) {
        const char* planet_name = data->Get("planet", "NO NAME SPECIFIED");
        entity_id_t index = GlobalGetState()->planets.GetIndexByName(planet_name);
        if (index < 0) {
            FAIL("Error while initializing ship '%s': no such planet '%s'", ship->name, planet_name)
        }
        ship->parent_planet = index;
    }

    ship->id = (entity_id_t) ship_entity;
    ship->Update();
    return (entity_id_t) ship_entity;
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
    ship_classes = (ShipClass*) malloc(sizeof(ShipClass) * ship_classes_count);
    for (int index=0; index < ship_classes_count; index++) {
        const DataNode* ship_data = data->GetArrayChild("ship_classes", index);
        ShipClass sc = {0};

        strncpy(sc.name, ship_data->Get("name", "[NAME MISSING]"), SHIPCLASS_NAME_MAX_SIZE);
        strncpy(sc.description, ship_data->Get("description", "[DESCRITION MISSING]"), SHIPCLASS_DESCRIPTION_MAX_SIZE);

        sc.max_capacity = ship_data->GetI("capacity", 0);
        sc.max_dv = ship_data->GetF("dv", 0) * 1000;  // km/s -> m/s
        sc.v_e = ship_data->GetF("Isp", 0) * 1000;    // km/s -> m/s
        sc.oem = ResourceCountsToKG(sc.max_capacity) / (exp(sc.max_dv/sc.v_e) - 1);
        ASSERT_ALOMST_EQUAL_FLOAT(sc.v_e * log((ResourceCountsToKG(sc.max_capacity) + sc.oem) / sc.oem), sc.max_dv)   // Remove when we're sure thisworks

        ship_classes[index] = sc;
        auto pair = ship_classes_ids.insert_or_assign(ship_data->Get("id", "_"), index);
        ship_classes[index].id = pair.first->first.c_str();  // points to string in dictionary
    }
    return ship_classes_count;
}

Ship* Ships::GetShip(entity_id_t id) const {
    if ((!alloc.IsValidIndex((int)id))) {
        FAIL("Invalid id (%d)", id)
    }
    return (Ship*) alloc.Get((int)id);
}

shipclass_index_t Ships::GetShipClassIndexById(const char *id) const { 
    auto find = ship_classes_ids.find(id);
    if (find == ship_classes_ids.end()) {
        ERROR("No such ship id '%s'", id)
        return BUILDING_INDEX_INVALID;
    }
    return find->second;
}

const ShipClass* Ships::GetShipClassByIndex(shipclass_index_t index) const {
#ifndef LOGGING_DISABLE
    if (ship_classes == NULL) {
        ERROR("Ship Class uninitialized")
        return NULL;
    }
    if (index >= ship_classes_count) {
        ERROR("Invalid ship class index (%d >= %d or negative)", index, ship_classes_count)
        return NULL;
    }
#endif
    return &ship_classes[index];
}

Ship* GetShip(entity_id_t uuid) {
    return GlobalGetState()->ships.GetShip(uuid);
}

const ShipClass* GetShipClassByIndex(shipclass_index_t index) {
    return GlobalGetState()->ships.GetShipClassByIndex(index);
}

int LoadShipClasses(const DataNode* data) {
    return GlobalGetState()->ships.LoadShipClasses(data);
}
