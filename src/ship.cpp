#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"

TransportContainer::TransportContainer() {
    type = NONE;
    content.quest = GetInvalidId();
}

TransportContainer::TransportContainer(entity_id_t quest) {
    type = QUEST;
    content.quest = quest;
}

TransportContainer::TransportContainer(ResourceTransfer resource_transfer) {
    type = RESOURCE;
    content.resource_transfer = resource_transfer;
}

resource_count_t TransportContainer::GetMass() const {
    switch (type) {
    case TransportContainer::NONE: break;
    case QUEST:
        return GetQuestPtr()->payload_mass;
    case RESOURCE:
        return content.resource_transfer.quantity;
    }
    return 0;
}

Quest* TransportContainer::GetQuestPtr() const {
    if (type != QUEST) return NULL;
    return GlobalGetState()->quest_manager._active_quests[content.quest];
}

double ShipClass::GetPayloadCapacity(double dv) const {
    // dv = v_e * ln(1 + m_fuel/m_dry)
    // fuel_ratio = (exp(dv/v_e) - 1) / (exp(dv_max/v_e) - 1)
    // MOTM * fuel_ratio - OEM

    // v_e
    // dv_max
    // m_fuel_max = cargo_cap
    // OEM = cargo_cap/(exp(dv_max/v_e) -1)
    //double oem = max_capacity/max_fuel_ratio;
    double fuel_ratio = (exp(dv/v_e) - 1) / (exp(max_dv/v_e) - 1);
    return (1 - fuel_ratio) * max_capacity;
}

double ShipClass::GetFuelRequiredFull(double dv) const {
    // assuming max payload
    double fuel_ratio = (exp(dv/v_e) - 1) / (exp(max_dv/v_e) - 1);
    return fuel_ratio * max_capacity;
}

double ShipClass::GetFuelRequiredEmpty(double dv) const {
    // assuming no payload
    // dv = v_e * ln(1 + m_fuel/m_dry)
    // m_fuel = oem * (exp(dv/V_e) - 1)
    // max_capacity = oem * (exp(dv_max/V_e) - 1)
    // oem = max_capacity / (exp(dv_max/V_e) - 1)
    double oem = max_capacity / (exp(max_dv/v_e) - 1);
    return oem * (exp(dv/v_e) - 1);
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

    Time min_time = 0;
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
    payload = std::vector<TransportContainer>();
}

void Ship::Serialize(DataNode *data) const {
    data->Set("name", name);
    data->Set("is_parked", is_parked ? "y" : "n");
    
    data->Set("class_id", GetShipClassByIndex(ship_class)->id);

    // Not necaissarily the same as ammount specified in the transfer
    if (payload.size() > 0) {
        data->SetArray("payload", payload.size());
    }
    for (int i=0; i < payload.size(); i++) {
        data->SetArrayElemI("payload_type", i, payload[i].type);
        switch (payload[i].type) {
        case TransportContainer::NONE: break;
        case TransportContainer::QUEST:
            data->SetArrayElemI("quest", i, payload[i].content.quest);
            break;
        case TransportContainer::RESOURCE:
            data->SetArrayElemI("resource_type", i, payload[i].content.resource_transfer.resource_id);
            data->SetArrayElemF("quantity", i, payload[i].content.resource_transfer.quantity / 1000);
            break;
        }
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

    payload.resize(data->GetArrayLen("payload", true));
    for (int i=0; i < payload.size(); i++) {
        switch (payload[i].type) {
        case TransportContainer::NONE: break;
        case TransportContainer::QUEST:
            payload[i].content.quest = (entity_id_t) data->GetArrayI("quest", i);
            break;
        case TransportContainer::RESOURCE:
            payload[i].content.resource_transfer.resource_id = (ResourceType) data->GetArrayI("type", i);
            payload[i].content.resource_transfer.quantity = data->GetArrayF("quantity", i) * 1000;
            break;
        }
    }

    color = PALETTE_GREEN;

    prepared_plans_count = data->GetArrayChildLen("prepared_plans", true);
    for (int i=0; i < prepared_plans_count; i++) {
        prepared_plans[i] = TransferPlan();
        prepared_plans[i].Deserialize(data->GetArrayChild("prepared_plans", i, true));
    }
}

resource_count_t Ship::GetPayloadMass() const{
    resource_count_t res = 0;
    for(TransportContainer cont : payload) {
        res += cont.GetMass();
    }
    return res;
}

resource_count_t Ship::GetMaxCapacity() const {
    return GetShipClassByIndex(ship_class)->max_capacity;
}

double Ship::GetRemainingPayloadCapacity(double dv) const {
    resource_count_t capacity = GetShipClassByIndex(ship_class)->GetPayloadCapacity(dv);
    //DEBUG_SHOW_F(GetShipClassByIndex(ship_class)->GetPayloadCapacity(0))
    //DEBUG_SHOW_F(capacity)
    return capacity - GetPayloadMass();
}

double Ship::GetFuelRequiredEmpty(double dv) const {
    const ShipClass* sc = GetShipClassByIndex(ship_class);
    double oem = sc->max_capacity / (exp(sc->max_dv/sc->v_e) - 1);
    return (oem + GetPayloadMass()) * (exp(dv/sc->v_e) - 1);
}

double Ship::GetCapableDV() const {
    // >> BUG IN HERE <<
    const ShipClass* sc = GetShipClassByIndex(ship_class);
    double oem = sc->max_capacity / (exp(sc->max_dv/sc->v_e) - 1);
    return sc->v_e * log((sc->max_capacity + oem) / (GetPayloadMass() + oem));

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
    payload.push_back(TransportContainer(tp.resource_transfer));
    INFO("Assigning transfer plan %f to ship %s", tp.arrival_time, name);
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
    Time min_time = 0;
    if (prepared_plans_count == 0) {
        prepared_plans[prepared_plans_count].departure_planet = parent_planet;
        min_time = GlobalGetNow();
    } else {
        prepared_plans[prepared_plans_count].departure_planet = prepared_plans[prepared_plans_count - 1].arrival_planet;
        min_time = prepared_plans[prepared_plans_count - 1].arrival_time;
    }

    plan_edit_index = index;
    tp_ui.SetPlan(&prepared_plans[index], id, min_time, 1e20);
}

void Ship::Update() {
    Time now = GlobalGetNow();

    if (prepared_plans_count == 0 || (plan_edit_index == 0 && prepared_plans_count == 1)) {
        position = GetPlanet(parent_planet)->position;
    } else {
        const TransferPlan& tp = prepared_plans[0];
        if (is_parked) {
            if (TimeIsEarlier(tp.departure_time, now)) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(parent_planet)->position;
            }
        } else {
            if (TimeIsEarlier(tp.arrival_time, now)) {
                _OnArrival(tp);
            } else {
                position = OrbitGetPosition(&tp.transfer_orbit[tp.primary_solution], now);
            }
        }
    }

    draw_pos = GetScreenTransform()->TransformV(position.cartesian);
    if (is_parked) {
        double rad = fmax(GetScreenTransform()->TransformS(GetPlanet(parent_planet)->radius), 4) + 8.0;
        double phase = 20.0 /  rad * index_on_planet;
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
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
        OrbitPos to_departure = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            TimeLatest(plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            plan.arrival_time
        );
        DrawOrbitBounded(&plan.transfer_orbit[plan.primary_solution], to_departure, to_arrival, 0, 
            ColorAlpha(color, i == highlighted_plan_index ? 1 : 0.5)
        );
        if (i == plan_edit_index){
            DrawOrbitBounded(&plan.transfer_orbit[plan.primary_solution], to_departure, to_arrival, 0, MAIN_UI_COLOR);
        }
    }
    if (plan_edit_index >= 0 && IsIdValid(prepared_plans[plan_edit_index].arrival_planet)) {
        const TransferPlan& last_tp = prepared_plans[plan_edit_index];
        OrbitPos last_pos = OrbitGetPosition(
            &GetPlanet(last_tp.arrival_planet)->orbit, 
            last_tp.arrival_time
        );
        Vector2 last_draw_pos = c_transf->TransformV(last_pos.cartesian);
        _DrawShipAt(last_draw_pos, ColorAlpha(color, 0.5));
    }
}

void _UIDrawTransferplans(Ship* ship) {
    Time now = GlobalGetNow();
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
            (int) TimeDays(TimeSub(ship->prepared_plans[i].arrival_time, now)),
            ((int) TimeSeconds(TimeSub(ship->prepared_plans[i].arrival_time, now)) % 86400) / 3600
        );

        snprintf(tp_str[1], 40, "  %s >> %s", departure_planet_name, arrival_planet_name);

        // Double Button
        UIContextPushInset(2, UIContextCurrent().GetLineHeight() * 2);
        UIContextPushHSplit(0, -32);
        UIContextEnclose(BG_COLOR, PALETTE_BLUE);
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
            UIContextEnclose(BG_COLOR, PALETTE_BLUE);
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

    resource_count_t max_mass = ship->GetMaxCapacity() - ship->GetPayloadMass();
    if (ship->is_parked) {
        for(auto it = qm->_active_quests.GetIter(); it; it++) {
            Quest* quest = qm->_active_quests.Get(it);
            if (quest->departure_planet != ship->parent_planet) continue;
            bool can_accept = quest->payload_mass <= max_mass;
            auto quest_in_cargo = ship->payload.end();
            for(auto it2=ship->payload.begin(); it2 != ship->payload.end(); it2++) {
                if (it2->type == TransportContainer::QUEST && it2->content.quest == it.index) {
                    quest_in_cargo = it2;
                }
            }
            ButtonStateFlags button_state = quest->DrawUI(true, quest_in_cargo != ship->payload.end());
            if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED && can_accept) {
                if (quest_in_cargo != ship->payload.end()) {
                    qm->PutbackQuest(ship, it.index);
                } else {
                    qm->PickupQuest(ship, it.index);
                }
            }
        }
    }
}

void Ship::DrawUI(const CoordinateTransform* c_transf) {
    //float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), draw_pos);
    if (mouse_hover) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, RED);
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
        text_size, MAIN_UI_COLOR
    );

    UIContextEnclose(BG_COLOR, color);

    StringBuilder sb;
    sb.AddLine(name);
    sb.AddFormat("Payload %2.3f / %2.3f kT\n", GetPayloadMass() / 1e6, GetMaxCapacity() / 1e6);
    sb.AddFormat("I_sp        %2.2f km/s\n", GetShipClassByIndex(ship_class)->v_e / 1000);
    sb.AddFormat("dv  left    %2.2f km/s\n", GetCapableDV());
    UIContextWrite(sb.c_str);
    UIContextFillline(GetPayloadMass() / GetMaxCapacity(), MAIN_UI_COLOR, BG_COLOR);

    _UIDrawTransferplans(this);
    _UIDrawQuests(this);
}

void Ship::Inspect() {
}

void Ship::_OnDeparture(const TransferPlan& tp) {
    GetPlanet(tp.departure_planet)->economy.DrawResource(RESOURCE_WATER, tp.fuel_mass);

    char date_buffer[30];
    FormatTime(date_buffer, 30, tp.departure_time);
    for(TransportContainer cont : payload) {
        switch (cont.type) {
        case TransportContainer::NONE: break;
        case TransportContainer::QUEST:
            USER_INFO(":: On %s, \"%s\" picked up quest on %s", 
                date_buffer,
                name,
                GetPlanet(parent_planet)->name
            );
            break;
        case TransportContainer::RESOURCE:
            USER_INFO(":: On %s, \"%s\" picked up %f kg of %s on %s", 
                date_buffer,
                name,
                cont.content.resource_transfer.quantity,
                GetResourceData(cont.content.resource_transfer.resource_id).name,
                GetPlanet(parent_planet)->name
            );
            break;
        }
    }

    parent_planet = GetInvalidId();
    is_parked = false;

    Update();
}

void Ship::_OnArrival(const TransferPlan& tp) {
    parent_planet = tp.arrival_planet;
    is_parked = true;
    position = GetPlanet(parent_planet)->position;
    for(int i=0; i < payload.size(); i++) {
        TransportContainer cont = payload[i];
        switch (cont.type) {
        case TransportContainer::NONE: break;
        case TransportContainer::QUEST:{
            const Quest* quest = GlobalGetState()->quest_manager._active_quests.Get(cont.content.quest);
            if (quest->arrival_planet == tp.arrival_planet) {
                payload.erase(payload.begin() + i--);
                GlobalGetState()->quest_manager.CompleteQuest(cont.content.quest);
            }
        break;}
        case TransportContainer::RESOURCE:{
            ResourceTransfer rt = cont.content.resource_transfer;
            payload.erase(payload.begin() + i--);
            resource_count_t delivered = GetPlanet(tp.arrival_planet)->economy.GiveResource(rt.resource_id, rt.quantity);  // Ignore how much actually arrives (for now)

            char date_buffer[30];
            FormatTime(date_buffer, 30, tp.arrival_time);
            USER_INFO(":: On %s, \"%s\" delivered %f kg of %s to %s", 
                date_buffer,
                name,
                delivered,
                GetResourceData(rt.resource_id).name,
                GetPlanet(parent_planet)->name
            );
        break;}
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
    //printf("Adding Ship N°%d\n", index)
    //entity_map.insert({uuid, ship_entity});
    Ship *ship;
    int ship_entity = alloc.Allocate(&ship);
    
    ship->Deserialize(data);
    if (ship->is_parked) {
        const char* planet_name = data->Get("planet", "NO NAME SPECIFIED");
        const Planet* planet = GlobalGetState()->planets.GetPlanetByName(planet_name);
        if (planet == NULL) {
            FAIL("Error while initializing ship '%s': no such planet '%s'", ship->name, planet_name)
        }
        ship->parent_planet = planet->id;
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

        sc.max_capacity = ship_data->GetF("capacity", 0) * 1000;  // t -> kg
        sc.max_dv = ship_data->GetF("dv", 0) * 1000;  // km/s -> m/s
        sc.v_e = ship_data->GetF("Isp", 0) * 1000;    // km/s -> m/s

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
