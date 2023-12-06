#include "quests.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

void _GenerateRandomQuest(Quest* quest) {
    Time now = GlobalGetNow();

    // There are always N accessible, plausibly profitable  quests
    // ... And M taunting ones
    // Transport quests: Payouts in cash are proportional to the players tech, dv involved and payload
    // Research quests: payouts in tech/items when reaching far away places (more interesting with refueling)

    quest->payload_mass = GetRandomValue(0, 1e6);
    quest->payout = GetRandomValue(0, 1e6);
    quest->pickup_expiration_time = TimeAddSec(now, 86400 * 5);
    quest->delivery_expiration_time = TimeAddSec(now, 86400 * 10);
    quest->departure_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
    do quest->arrival_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
    while (quest->arrival_planet == quest->departure_planet);

}


// ========================================
//                  Quest
// ========================================


Quest::Quest() {
    is_accepted = false;
    is_ins_transit = false;

    departure_planet = GetInvalidId();
    arrival_planet = GetInvalidId();

    payload_mass = 0;
    pickup_expiration_time = GetInvalidTime();
    delivery_expiration_time = GetInvalidTime();

    payout = 0;
}

void Quest::CopyFrom(const Quest* other) {
    is_accepted = other->is_accepted;
    is_ins_transit = other->is_ins_transit;

    departure_planet = other->departure_planet;
    arrival_planet = other->arrival_planet;

    payload_mass = other->payload_mass;
    pickup_expiration_time = other->pickup_expiration_time;
    delivery_expiration_time = other->delivery_expiration_time;

    payout = other->payout;
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
    sb.Add(GetPlanet(departure_planet)->name).Add(" >> ").Add(GetPlanet(arrival_planet)->name).Add("  ").AddF(payload_mass / 1e6).AddLine("kT");
    sb.Add("Expires in ").AddTime(TimeSub(pickup_expiration_time, GlobalGetNow()));
    sb.Add("(").AddTime(TimeSub(delivery_expiration_time, GlobalGetNow())).AddLine(")");
    UIContextWrite(sb.c_str);

    UIContextPop();
    return button_state;
}

// ========================================
//              QuestManager
// ========================================

QuestManager::QuestManager() {
    _active_quests.Init();
}

void QuestManager::Serialize(DataNode* data) const {
    // TBD
}

void QuestManager::Deserialize(const DataNode* data) {
    // TBD
}

void QuestManager::Make() {
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        _GenerateRandomQuest(&_available_quests[i]);
    }
}

void QuestManager::Update(double dt) {
    Time now = GlobalGetNow();

    if (IsKeyPressed(KEY_Q)) show_ui = !show_ui;
    
    // Remove all expired and completed quests
    for(auto i = _active_quests.GetIter(); i; i++) {
        Quest* quest = _active_quests[i];
        if (TimeIsEarlier(quest->pickup_expiration_time, now) && !quest->is_ins_transit) {
            _active_quests.Erase(i);
        }
        if (TimeIsEarlier(quest->delivery_expiration_time, now)) {
            _active_quests.Erase(i);
        }
        if (quest->IsReadyForCompletion()) {
            CompleteQuest(i.index);
            _active_quests.Erase(i);
        }
    }
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        if (TimeIsEarlier(_available_quests[i].pickup_expiration_time, now)) {
            _GenerateRandomQuest(&_available_quests[i]);
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
    if (_active_quests.Count() == 0) {
        UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    }
    for(auto i = _active_quests.GetIter(); i; i++) {
        _active_quests[i]->DrawUI(false, _available_quests[i].is_ins_transit);
    }
    UIContextPop();  // HSplit

    UIContextPushHSplit(w/2, w);
    UIContextShrink(5, 5);
    // Available Quests
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        if(_available_quests[i].DrawUI(true, _available_quests[i].is_ins_transit) & BUTTON_STATE_FLAG_JUST_PRESSED) {
            AcceptQuest(i);
        }
    }

    UIContextPop();  // HSplit
}

void QuestManager::AcceptQuest(entity_id_t quest_index) {
    Quest* q;
    _active_quests.Allocate(&q);
    q->CopyFrom(&_available_quests[quest_index]);
    q->is_accepted = true;
    _GenerateRandomQuest(&_available_quests[quest_index]);
}

void QuestManager::PickupQuest(entity_id_t ship_index, entity_id_t quest_index) {
    Ship* ship = GetShip(ship_index);
    _active_quests[quest_index]->is_ins_transit = true;
    ship->payload.push_back(TransportContainer(quest_index));
}

void QuestManager::PutbackQuest(entity_id_t ship_index, entity_id_t quest_index) {
    Ship* ship = GetShip(ship_index);
    auto quest_in_cargo = ship->payload.end();
    for(auto it2=ship->payload.begin(); it2 != ship->payload.end(); it2++) {
        if (it2->type == TransportContainer::QUEST && it2->content.quest == quest_index) {
            quest_in_cargo = it2;
        }
    }
    if (quest_in_cargo == ship->payload.end()) {
        ERROR("Quest %d not currently on ship '%s'", quest_index, ship->name)
    }
    ship->payload.erase(quest_in_cargo);
    _active_quests[quest_index]->is_ins_transit = false;
}

void QuestManager::CompleteQuest(entity_id_t quest_index) {
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