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

struct QuestTemplate {
    std::vector<entity_id_t> departure_options = std::vector<entity_id_t>();
    std::vector<entity_id_t> destination_options = std::vector<entity_id_t>();
    double max_dv;
    double payload;
    double payout;

    entity_id_t GetRandomDeparturePlanet() const;
    entity_id_t GetRandomArrivalPlanet(entity_id_t departure_planet) const;
};

struct Quest {
    entity_id_t departure_planet;
    entity_id_t arrival_planet;
    entity_id_t current_planet;

    entity_id_t ship;

    double payload_mass;
    timemath::Time pickup_expiration_time;
    timemath::Time delivery_expiration_time;

    cost_t payout;

    Quest();
    void CopyFrom(const Quest* other);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    bool IsReadyForCompletion() const;
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;
};

#define AVAILABLE_QUESTS_NUM 5

struct QuestManager {
    Quest available_quests[AVAILABLE_QUESTS_NUM];
    IDAllocatorList<Quest> active_quests;

    QuestTemplate* templates;
    int template_count;

    bool show_ui;
    
    QuestManager();
    ~QuestManager();
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void Make();
    void Update(double dt);
    void Draw();

    void AcceptQuest(entity_id_t quest_index);
    void PickupQuest(entity_id_t ship_index, entity_id_t quest_index);
    void PutbackQuest(entity_id_t ship_index, entity_id_t quest_index);
    void QuestDepartedFrom(entity_id_t quest_index, entity_id_t planet_index);
    void QuestArrivedAt(entity_id_t quest_index, entity_id_t planet_index);
    void CompleteQuest(entity_id_t quest_index);

    int LoadQuests(const DataNode*);
    int RandomTemplateIndex();

    cost_t CollectPayout();
};

int LoadQuests(const DataNode*);

#endif  //QUESTS_H