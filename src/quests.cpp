#include "quests.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

const int QUEST_PANEL_HEIGHT = 64;

void _GenerateRandomQuest(Quest* quest, const QuestTemplate* quest_template) {
    timemath::Time now = GlobalGetNow();

    // There are always N accessible, plausibly profitable  quests
    // ... And M taunting ones
    // Transport quests: Payouts in cash are proportional to the players tech, dv involved and payload
    // Research quests: payouts in tech/items when reaching far away places (more interesting with refueling)
    // Quests appear unpredictably and player is forced to consider them in a short timeframe

    quest->payload_mass = quest_template->payload;
    quest->payout = quest_template->payout;
    quest->departure_planet = quest_template->GetRandomDeparturePlanet();
    quest->arrival_planet = quest_template->GetRandomArrivalPlanet(quest->departure_planet);
    double period_1 = GetPlanet(quest->departure_planet)->orbit.GetPeriod().Seconds();
    double period_2 = GetPlanet(quest->arrival_planet  )->orbit.GetPeriod().Seconds();
    double characteristic_period = fmax(1.0 / fabs(1.0 / period_1 - 1.0 / period_2), fmax(period_1, period_2) * 0.5);
    //int start_quarter_days = (int) GetRandomValue(4*4, 6*4);
    double add_time = GetRandomGaussian(characteristic_period * 2.0, characteristic_period);
    if (add_time < characteristic_period * 0.5)
        add_time = characteristic_period;
    quest->pickup_expiration_time = now + GetRandomGaussian(characteristic_period * 2.0, characteristic_period);
    quest->delivery_expiration_time = quest->pickup_expiration_time;
}

void _ClearQuest(Quest* quest) {
    quest->payload_mass = 0;
    quest->payout = 0;
    quest->departure_planet = GetInvalidId();
    quest->arrival_planet = GetInvalidId();
}


// ========================================
//              Quest Template
// ========================================


entity_id_t QuestTemplate::GetRandomDeparturePlanet() const {
    int selector = GetRandomValue(0, departure_options.size() - 1);
    return departure_options[selector];
}

entity_id_t QuestTemplate::GetRandomArrivalPlanet(entity_id_t departure_planet) const {
    entity_id_t destination_planet;
    double dv1, dv2;
    int iter_count = 0;
    do {
        int selector = GetRandomValue(0, destination_options.size() - 1);
        destination_planet = destination_options[selector];
        HohmannTransfer(
            &GetPlanet(departure_planet)->orbit,
            &GetPlanet(destination_planet)->orbit,
            GlobalGetNow(),
            NULL, NULL,
            &dv1, &dv2
        );
        if (iter_count++ > 1000) {
            ERROR("Quest Generation: Arrival planet finding exceeds iteration Count (>1000)")
            INFO("%f + %f ><? %f", dv1, dv2, max_dv)
            return departure_planet;  // Better broken quest than infinite freeze
        }
    } while (
        destination_planet == departure_planet || dv1 + dv2 > max_dv
    );
    //INFO("%d => %d :: %f + %f < %f", departure_planet, destination_planet, dv1, dv2, max_dv)
    return destination_planet;
}

// ========================================
//                  Quest
// ========================================


Quest::Quest() {
    departure_planet = GetInvalidId();
    arrival_planet = GetInvalidId();
    current_planet = GetInvalidId();
    ship = GetInvalidId();

    payload_mass = 0;
    pickup_expiration_time = timemath::Time::GetInvalid();
    delivery_expiration_time = timemath::Time::GetInvalid();

    payout = 0;
}

void Quest::CopyFrom(const Quest* other) {
    departure_planet = other->departure_planet;
    arrival_planet = other->arrival_planet;
    current_planet = other->current_planet;

    payload_mass = other->payload_mass;
    pickup_expiration_time = other->pickup_expiration_time;
    delivery_expiration_time = other->delivery_expiration_time;

    payout = other->payout;
}

void Quest::Serialize(DataNode* data) const {
    data->SetI("departure_planet", departure_planet);
    data->SetI("arrival_planet", arrival_planet);
    data->SetI("current_planet", current_planet);
    data->SetI("ship", ship);
    data->SetF("payload_mass", payload_mass);
    data->SetDate("pickup_expiration_time", pickup_expiration_time);
    data->SetDate("delivery_expiration_time", delivery_expiration_time);
    data->SetF("payout", payout);
}

void Quest::Deserialize(const DataNode* data) {
    departure_planet =          data->GetI("departure_planet", departure_planet);
    arrival_planet =            data->GetI("arrival_planet", arrival_planet);
    current_planet =            data->GetI("current_planet", current_planet);
    ship =                      data->GetI("ship", ship);
    payload_mass =              data->GetF("payload_mass", payload_mass);
    pickup_expiration_time =    data->GetDate("pickup_expiration_time", pickup_expiration_time);
    delivery_expiration_time =  data->GetDate("delivery_expiration_time", delivery_expiration_time);
    payout =                    data->GetF("payout", payout);
}

bool Quest::IsValid() const {
    return IsIdValid(departure_planet) && IsIdValid(arrival_planet);
}

ButtonStateFlags Quest::DrawUI(bool show_as_button, bool highlinght) const {
    // Assumes parent UI Context exists
    // Resturns if player wants to accept
    if (!IsValid()) {
        return BUTTON_STATE_FLAG_NONE;
    }

    int height = UIContextPushInset(3, QUEST_PANEL_HEIGHT);

    if (height == 0) {
        UIContextPop();
        return false;
    }
    if (highlinght) {
        UIContextEnclose(BG_COLOR, TRANSFER_UI_COLOR);
    } else {
        UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    }
    UIContextShrink(6, 6);
    ButtonStateFlags button_state = UIContextAsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    }
    if (height != QUEST_PANEL_HEIGHT) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    StringBuilder sb_mouse;
    double dv1, dv2;
    HohmannTransfer(&GetPlanet(departure_planet)->orbit, &GetPlanet(arrival_planet)->orbit, GlobalGetNow(), NULL, NULL, &dv1, &dv2);
    sb_mouse.AddFormat("DV: %.3f km/s\n", (dv1 + dv2) / 1000);

    StringBuilder sb;
    // Line 1
    sb.AddFormat("%s >> %s  %d cts", GetPlanet(departure_planet)->name, GetPlanet(arrival_planet)->name, KGToResourceCounts(payload_mass));
    if (button_state == BUTTON_STATE_FLAG_HOVER) {
        UISetMouseHint(sb_mouse.c_str);
    }
    if (IsIdValid(current_planet)) {
        sb.Add("  Now: [").Add(GetPlanet(current_planet)->name).AddLine("]");
    } else sb.AddLine("");
    // Line 2
    bool is_in_transit = IsIdValid(ship) && !GetShip(ship)->is_parked;
    if (is_in_transit) {
        sb.Add("Expires in ").AddTime(delivery_expiration_time - GlobalGetNow());
    } else {
        sb.Add("Expires in ").AddTime(pickup_expiration_time - GlobalGetNow());
    }
    sb.AddFormat("  => ").AddCost(payout);
    UIContextWrite(sb.c_str);

    UIContextPop();
    return button_state;
}

// ========================================
//              QuestManager
// ========================================

QuestManager::QuestManager() {
    active_quests.Init();
}

QuestManager::~QuestManager() {
    delete[] templates;
}

void QuestManager::Serialize(DataNode* data) const {
    data->SetArrayChild("active_quests", active_quests.alloc_count);
    for(auto it = active_quests.GetIter(); it; it++) {
        active_quests.Get(it)->Serialize(data->SetArrayElemChild("active_quests", it.iterator, DataNode()));
    }
    data->SetArrayChild("available_quests", GetAvailableQuests());
    for(int i=0; i < GetAvailableQuests(); i++) {
        available_quests[i].Serialize(data->SetArrayElemChild("available_quests", i, DataNode()));
    }
}

void QuestManager::Deserialize(const DataNode* data) {
    active_quests.Clear();
    for(int i=0; i < data->GetArrayChildLen("active_quests"); i++) {
        active_quests.Get(active_quests.Allocate())->Deserialize(data->GetArrayChild("active_quests", i));
    }
    for(int i=0; i < data->GetArrayChildLen("available_quests") && i < GetAvailableQuests(); i++) {
        available_quests[i].Deserialize(data->GetArrayChild("available_quests", i));
    }
    for(int i=data->GetArrayChildLen("available_quests"); i < GetAvailableQuests(); i++) {
        available_quests[i] = Quest();  // Just in case
    }
}

void QuestManager::Make() {
    for(int i=0; i < GetAvailableQuests(); i++) {
        _GenerateRandomQuest(&available_quests[i], &templates[RandomTemplateIndex()]);
    }
}

void QuestManager::Update(double dt) {
    timemath::Time now = GlobalGetNow();

    if (IsKeyPressed(KEY_Q)) show_ui = !show_ui;
    
    // Remove all expired and completed quests
    for(auto i = active_quests.GetIter(); i; i++) {
        Quest* quest = active_quests[i];
        bool is_in_transit = IsIdValid(quest->ship) && !GetShip(quest->ship)->is_parked;
        if (quest->pickup_expiration_time < now && !is_in_transit) {
            active_quests.Erase(i);
        }
        if (quest->delivery_expiration_time < now) {
            active_quests.Erase(i);
        }
    }
    for(int i=0; i < GetAvailableQuests(); i++) {
        if (available_quests[i].pickup_expiration_time < now) {
            _GenerateRandomQuest(&available_quests[i], &templates[RandomTemplateIndex()]);
        }
    }

    if (GlobalGetState()->calendar.IsNewDay()) {
        for(int i=0; i < GetAvailableQuests(); i++) {
            _GenerateRandomQuest(&available_quests[i], &templates[RandomTemplateIndex()]);
        }
    }
}

int current_available_quests_scroll = 0;
void QuestManager::Draw() {
    if (!show_ui) return;
    
    int x_margin = MinInt(100, GetScreenWidth()*.1);
    int y_margin = MinInt(50, GetScreenWidth()*.1);
    int w = GetScreenWidth() - x_margin*2;
    int h = GetScreenHeight() - y_margin*2;
    UIContextCreate(x_margin, y_margin, w, h, 16, MAIN_UI_COLOR);
    UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    UIContextPushHSplit(0, w/2);
    UIContextShrink(5, 5);
    // Active quests
    if (active_quests.Count() == 0) {
        UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    }
    for(auto i = active_quests.GetIter(); i; i++) {
        active_quests[i]->DrawUI(false, IsIdValid(available_quests[i].ship));
    }
    UIContextPop();  // HSplit

    UIContextPushHSplit(w/2, w);
    UIContextShrink(5, 5);

    if (GlobalGetState()->current_focus == GlobalState::QUEST_MANAGER) {
        int max_scroll = MaxInt(QUEST_PANEL_HEIGHT * GetAvailableQuests() - UIContextCurrent().height, 0);
        current_available_quests_scroll = ClampInt(current_available_quests_scroll - GetMouseWheelMove() * 20, 0, max_scroll);
    }

    UIContextPushScrollInset(0, UIContextCurrent().height, QUEST_PANEL_HEIGHT * GetAvailableQuests(), current_available_quests_scroll);
    // Available Quests
    for(int i=0; i < GetAvailableQuests(); i++) {
        if(available_quests[i].DrawUI(true, IsIdValid(available_quests[i].ship)) & BUTTON_STATE_FLAG_JUST_PRESSED) {
            AcceptQuest(i);
        }
    }
    UIContextPop();  // ScrollInseet

    UIContextPop();  // HSplit
}

void QuestManager::AcceptQuest(entity_id_t quest_index) {
    Quest* q;
    active_quests.Allocate(&q);
    q->CopyFrom(&available_quests[quest_index]);
    q->current_planet = q->departure_planet;
    _ClearQuest(&available_quests[quest_index]);
}

void QuestManager::PickupQuest(entity_id_t ship_index, entity_id_t quest_index) {
    active_quests[quest_index]->ship = ship_index;
    //ship->payload.push_back(TransportContainer(quest_index));
}

void QuestManager::PutbackQuest(entity_id_t ship_index, entity_id_t quest_index) {
    Ship* ship = GetShip(ship_index);
    //auto quest_in_cargo = ship->payload.end();
    //for(auto it2=ship->payload.begin(); it2 != ship->payload.end(); it2++) {
    //    if (it2->type == TransportContainer::QUEST && it2->content.quest == quest_index) {
    //        quest_in_cargo = it2;
    //    }
    //}
    if (active_quests[quest_index]->ship != ship_index) {
        ERROR("Quest %d not currently on ship '%s'", quest_index, ship->name)
        return;
    }
    if (!ship->is_parked) {
        ERROR("'%s' must be parked on planet to deliver quest", quest_index, ship->name)
        return;
    }
    active_quests[quest_index]->ship = GetInvalidId();
    //ship->payload.erase(quest_in_cargo);
}

void QuestManager::QuestDepartedFrom(entity_id_t quest_index, entity_id_t planet_index) {
    active_quests[quest_index]->current_planet = GetInvalidId();
}

void QuestManager::QuestArrivedAt(entity_id_t quest_index, entity_id_t planet_index) {
    Quest* q = active_quests[quest_index];
    q->current_planet = planet_index;
    if (q->arrival_planet == planet_index) {
        CompleteQuest(quest_index);
    }
}

void QuestManager::CompleteQuest(entity_id_t quest_index) {
    Quest* q = active_quests[quest_index];
    INFO("Quest completed (M§M %f)", q->payout)
    GlobalGetState()->CompleteTransaction(q->payout, "Completed quest");
}

int QuestManager::GetAvailableQuests() const {
    return _AVAILABLE_QUESTS;
}

int QuestManager::LoadQuests(const DataNode* data) {
    delete[] templates;

    template_count = data->GetArrayChildLen("resource_missions");
    templates = new QuestTemplate[template_count];
    for(int i=0; i < template_count; i++) {
        const DataNode* mission_data = data->GetArrayChild("resource_missions", i);

        templates[i].payload = mission_data->GetF("payload", 0.0) * 1000;
        mission_data->Get("flavour", "UNFLAVOURED");
        for (int j=0; j < mission_data->GetArrayLen("departure"); j++) {
            entity_id_t planet = GlobalGetState()->planets.GetIndexByName(mission_data->GetArray("departure", j));
            templates[i].departure_options.push_back(planet);
        }

        for (int j=0; j < mission_data->GetArrayLen("destination"); j++) {
            entity_id_t planet = GlobalGetState()->planets.GetIndexByName(mission_data->GetArray("destination", j));
            templates[i].destination_options.push_back(planet);
        }
        templates[i].max_dv = INFINITY;
        for (int j=0; j < mission_data->GetArrayLen("ship_accesibilty"); j++) {
            entity_id_t ship_class = GlobalGetState()->ships.GetShipClassIndexById(mission_data->GetArray("ship_accesibilty", j));
            const ShipClass* sc = GetShipClassByIndex(ship_class);
            double max_dv_class = sc->v_e * log(ResourceCountsToKG(sc->max_capacity) + sc->oem) - sc->v_e * log(templates[i].payload + sc->oem);
            if (max_dv_class < templates[i].max_dv) templates[i].max_dv = max_dv_class;
        }

        // TODO: Check if quest is possible for each startere planet

        templates[i].payout = (int) mission_data->GetF("payout", 0.0);  // to allow for exponential notation etc.
    }
    return template_count;
}

int QuestManager::RandomTemplateIndex() {
    int res = GetRandomValue(0, template_count - 1);
    return res;
}


// ========================================
//                  General
// ========================================

int LoadQuests(const DataNode* data) {
    return GlobalGetState()->quest_manager.LoadQuests(data);
}