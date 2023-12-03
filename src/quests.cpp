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

    quest->payload_mass = GetRandomValue(0, 1000);
    quest->payout = GetRandomValue(0, 1000);
    quest->pickup_expiration_time = TimeAddSec(now, 86400 * 5);
    quest->delivery_expiration_time = TimeAddSec(now, 86400 * 10);
    quest->departure_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
    quest->arrival_planet = GetRandomValue(0, GlobalGetState()->planets.planet_count - 1);
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

void Quest::DrawUIGeneral() const {
    StringBuilder sb;
    sb.Add(GetPlanet(departure_planet)->name).Add(" >> ").Add(GetPlanet(arrival_planet)->name).Add("  ").AddF(payload_mass).AddLine("kT");
    sb.Add("Expires in ").AddTime(TimeSub(pickup_expiration_time, GlobalGetNow()));
    sb.Add("(").AddTime(TimeSub(delivery_expiration_time, GlobalGetNow())).AddLine(")");

    //std::stringstream ss;
    //ss << GetPlanet(departure_planet)->name << " >> " << GetPlanet(arrival_planet)->name << "   " << payload_mass << "kT\n";
    //ss << "Expires" <<  << GetPlanet(arrival_planet)->name << "   " << payload_mass << "kT\n";
    //ss << GetPlanet(departure_planet)->name << " >> " << GetPlanet(arrival_planet)->name << "   " << payload_mass << "kT\n";
    UIContextWrite(sb.c_str);
}


void Quest::DrawUIActive() const {
    // Assumes parent UI Context exists
    int height = UIContextPushInset(3, 50);
    if (height == 0) {
        UIContextPop(); return;
    }
    if (is_ins_transit) {
        UIContextEnclose(BG_COLOR, TRANSFER_UI_COLOR);
    } else {
        UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    }
    if (height != 50) {
        UIContextPop(); return;
    }

    DrawUIGeneral();
    UIContextPop();
}

bool Quest::DrawUIAvailable() const {
    // Assumes parent UI Context exists
    // Resturns if player wants to accept
    int height = UIContextPushInset(3, 50);
    if (height == 0) {
        UIContextPop();
        return false;
    }
    UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);
    ButtonStateFlags button_state = UIContextAsButton();
    HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    if (height != 50) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    DrawUIGeneral();

    UIContextPop();
    return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
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
        _active_quests[i]->DrawUIActive();
    }
    UIContextPop();  // HSplit

    UIContextPushHSplit(w/2, w);
    UIContextShrink(5, 5);
    // Available Quests
    for(int i=0; i < AVAILABLE_QUESTS_NUM; i++) {
        if(_available_quests[i].DrawUIAvailable()) {
            AcceptQuest(i);
        }
    }

    UIContextPop();  // HSplit
}

void QuestManager::AcceptQuest(int quest_index){
    Quest* q;
    _active_quests.Allocate(&q);
    q->CopyFrom(&_available_quests[quest_index]);
    q->is_accepted = true;
    _GenerateRandomQuest(&_available_quests[quest_index]);
}

void QuestManager::PickupQuest(int quest_index){
    _active_quests[quest_index]->is_ins_transit = true;
}

void QuestManager::CompleteQuest(int quest_index){
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