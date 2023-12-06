#include "quests.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

void _GenerateRandomQuest(Quest* quest) {
    timemath::Time now = GlobalGetNow();

    // There are always N accessible, plausibly profitable  quests
    // ... And M taunting ones
    // Transport quests: Payouts in cash are proportional to the players tech, dv involved and payload
    // Research quests: payouts in tech/items when reaching far away places (more interesting with refueling)

    quest->payload_mass = GetRandomValue(0, 1e6);
    quest->payout = GetRandomValue(0, 1e6);
    quest->pickup_expiration_time = timemath::TimeAddSec(now, 86400 * 5);
    quest->delivery_expiration_time = timemath::TimeAddSec(now, 86400 * 10);
    quest->departure_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
    do quest->arrival_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
    while (quest->arrival_planet == quest->departure_planet);

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
    pickup_expiration_time = timemath::GetInvalidTime();
    delivery_expiration_time = timemath::GetInvalidTime();

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

bool Quest::IsReadyForCompletion() const {
    return false;
}

ButtonStateFlags Quest::DrawUI(bool show_as_button, bool highlinght) const {
    // Assumes parent UI Context exists
    // Resturns if player wants to accept
    int height = UIContextPushInset(3, 50);
    if (height == 0) {
        UIContextPop();
        return false;
    }
    if (highlinght) {
        UIContextEnclose(BG_COLOR, TRANSFER_UI_COLOR);
    } else {
        UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    }
    ButtonStateFlags button_state = UIContextAsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    }
    if (height != 50) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    StringBuilder sb;
    // Line 1
    sb.Add(GetPlanet(departure_planet)->name).Add(" >> ").Add(GetPlanet(arrival_planet)->name).Add("  ").AddF(payload_mass / 1e6).AddLine("kT");
    // Line 2
    sb.Add("Expires in ").AddTime(TimeSub(pickup_expiration_time, GlobalGetNow()));
    sb.Add("(").AddTime(TimeSub(delivery_expiration_time, GlobalGetNow())).AddLine(")");
    // Line 3
    if (IsIdValid(current_planet)) {
        sb.Add("[").Add(GetPlanet(current_planet)->name).AddLine("]");
    }
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

void QuestManager::Serialize(DataNode* data) const {
    data->SetArrayChild("active_quests", active_quests.alloc_count);
    for(auto it = active_quests.GetIter(); it; it++) {
        active_quests.Get(it)->Serialize(data->SetArrayElemChild("active_quests", it.iterator, DataNode()));
    }
    data->SetArrayChild("available_quests", AVAILABLE_QUESTS_NUM);
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        available_quests[i].Serialize(data->SetArrayElemChild("available_quests", i, DataNode()));
    }
}

void QuestManager::Deserialize(const DataNode* data) {
    active_quests.Clear();
    for(int i=0; i < data->GetArrayChildLen("active_quests"); i++) {
        active_quests.Get(active_quests.Allocate())->Deserialize(data->GetArrayChild("active_quests", i));
    }
    for(int i=0; i < data->GetArrayChildLen("available_quests") && i < AVAILABLE_QUESTS_NUM; i++) {
        available_quests[i].Deserialize(data->GetArrayChild("available_quests", i));
    }
    for(int i=data->GetArrayChildLen("available_quests"); i < AVAILABLE_QUESTS_NUM; i++) {
        available_quests[i] = Quest();  // Just in case
    }
}

void QuestManager::Make() {
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        _GenerateRandomQuest(&available_quests[i]);
    }
}

void QuestManager::Update(double dt) {
    timemath::Time now = GlobalGetNow();

    if (IsKeyPressed(KEY_Q)) show_ui = !show_ui;
    
    // Remove all expired and completed quests
    for(auto i = active_quests.GetIter(); i; i++) {
        Quest* quest = active_quests[i];
        bool is_in_transit = IsIdValid(quest->ship) && !GetShip(quest->ship)->is_parked;
        if (TimeIsEarlier(quest->pickup_expiration_time, now) && !is_in_transit) {
            active_quests.Erase(i);
        }
        if (TimeIsEarlier(quest->delivery_expiration_time, now)) {
            active_quests.Erase(i);
        }
        if (quest->IsReadyForCompletion()) {
            CompleteQuest(i.index);
            active_quests.Erase(i);
        }
    }
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        if (TimeIsEarlier(available_quests[i].pickup_expiration_time, now)) {
            _GenerateRandomQuest(&available_quests[i]);
        }
    }
}

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
    // Available Quests
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        if(available_quests[i].DrawUI(true, IsIdValid(available_quests[i].ship)) & BUTTON_STATE_FLAG_JUST_PRESSED) {
            AcceptQuest(i);
        }
    }

    UIContextPop();  // HSplit
}

void QuestManager::AcceptQuest(entity_id_t quest_index) {
    Quest* q;
    active_quests.Allocate(&q);
    q->CopyFrom(&available_quests[quest_index]);
    q->current_planet = q->departure_planet;
    _GenerateRandomQuest(&available_quests[quest_index]);
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
    if (q->arrival_planet = planet_index) {
        CompleteQuest(quest_index);
    }
}

void QuestManager::CompleteQuest(entity_id_t quest_index) {
    INFO("Quest completed")
    NOT_IMPLEMENTED
}

cost_t QuestManager::CollectPayout() {
    NOT_IMPLEMENTED
}


// ========================================
//                  General
// ========================================

int LoadQuests(const DataNode*) {
    return 0;
}