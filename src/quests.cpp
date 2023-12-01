#include "quests.hpp"
#include "logging.hpp"
#include "global_state.hpp"

//QuestManager quest_manager;

void _GenerateRandomQuest(Quest* quest) {
    NOT_IMPLEMENTED
}

// ========================================
//              QuestList
// ========================================

QuestList::QuestList() {
    alloc_count = 0;
    capacity = 32;
    array = (Quest*) malloc(sizeof(Quest) * capacity);
}

QuestList::~QuestList() {
    free(array);
}

void QuestList::Push(Quest q) {
    if (alloc_count >= capacity) {
        capacity += 32;
        array = (Quest*) realloc(array, sizeof(Quest) * capacity);
        for(int i = capacity-32; i < capacity; free_index_array[i] = i++);
    }
    array[free_index_array[alloc_count]] = q;
    alloc_count++;
}

void QuestList::Erase(int index) {
    alloc_count--;
    free_index_array[alloc_count] = index;
}

// ========================================
//              QuestManager
// ========================================

QuestManager::QuestManager() {
    _quests = QuestList();
}

void QuestManager::Update(double dt) {
    Time now = GlobalGetNow();
    for(int i=0; i < _quests.alloc_count; i++) {
        Quest* quest = &_quests.array[i];
        if (quest->is_accepted) {
            if (TimeIsEarlier(quest->pickup_expiration_time, now) && !quest->is_ins_transit) {

            }
            if (TimeIsEarlier(quest->delivery_expiration_time, now)) {
                
            }
        }
    }
}

cost_t QuestManager::CollectPayout() {
    NOT_IMPLEMENTED
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

const char* Quest::GetDescription() const {
    NOT_IMPLEMENTED
}

// ========================================
//                  General
// ========================================


QuestManager* GetQuestManager() {
    return NULL;//return &quest_manager;
}

int LoadQuests(const DataNode*) {
    return 0;
}