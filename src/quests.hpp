#ifndef QUESTS_H
#define QUESTS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "time.hpp"

struct Quest;
struct QuestList {  // List where indicies are preserved
    Quest* array;
    int* free_index_array;
    int alloc_count;
    int capacity;

    QuestList();
    ~QuestList();
    void Push(Quest quest);
    void Erase(int index);
};

struct QuestManager {
    QuestList _quests;
    
    QuestManager();
    void Update(double dt);

    //int GetAvailableQuestCount() const;
    //const Quest* GetAvailableQuest(int index) const;
    //int GetActiveQuestCount() const;
    //const Quest* GetActiveQuest(int index) const;

    void AcceptQuest(int quest);
    void PickupQuest(int quest);
    void CompleteQuest(int quest);

    cost_t CollectPayout();
};

struct Quest {
    bool is_accepted;
    bool is_ins_transit;

    entity_id_t departure_planet;
    entity_id_t arrival_planet;

    double payload_mass;
    Time pickup_expiration_time;
    Time delivery_expiration_time;

    cost_t payout;

    Quest();
    const char* GetDescription() const;
};

QuestManager* GetQuestManager();

int LoadQuests(const DataNode*);

#endif  //QUESTS_H