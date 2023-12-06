#ifndef QUESTS_H
#define QUESTS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "time.hpp"
#include "id_allocator.hpp"
#include "datanode.hpp"
#include "ui.hpp"

struct Ship;

struct Quest {
    bool is_accepted;
    bool is_ins_transit;

    entity_id_t departure_planet;
    entity_id_t arrival_planet;

    entity_id_t ship;

    double payload_mass;
    Time pickup_expiration_time;
    Time delivery_expiration_time;

    cost_t payout;

    Quest();
    void CopyFrom(const Quest* other);
    bool IsReadyForCompletion() const;

    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;
};

#define AVAILABLE_QUESTS_NUM 20

struct QuestManager {
    Quest _available_quests[AVAILABLE_QUESTS_NUM];
    IDAllocatorList<Quest> _active_quests;

    bool show_ui;
    
    QuestManager();
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void Make();
    void Update(double dt);
    void Draw();

    void AcceptQuest(entity_id_t quest_index);
    void PickupQuest(entity_id_t ship_index, entity_id_t quest_index);
    void PutbackQuest(entity_id_t ship_index, entity_id_t quest_index);
    void CompleteQuest(entity_id_t quest_index);

    cost_t CollectPayout();
};

int LoadQuests(const DataNode*);

#endif  //QUESTS_H