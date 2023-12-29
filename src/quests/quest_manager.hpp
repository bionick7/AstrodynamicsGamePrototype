#ifndef QUEST_MANAGER_H
#define QUEST_MANAGER_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "time.hpp"
#include "id_allocator.hpp"
#include "datanode.hpp"
#include "ui.hpp"
#include "quest.hpp"
#include "wren_interface.hpp"

struct Ship;
#define _AVAILABLE_QUESTS 20

struct QuestManager {
    Quest available_quests[_AVAILABLE_QUESTS];
    IDAllocatorList<Task> active_tasks;
    IDAllocatorList<Quest> active_quests;

    //QuestTemplate* templates;
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
    entity_id_t CreateTask(entity_id_t quest_index);
    int GetAvailableQuests() const;

    void PickupTask(entity_id_t ship_index, entity_id_t task_index);
    void PutbackTask(entity_id_t ship_index, entity_id_t task_index);
    void TaskDepartedFrom(entity_id_t task_index, entity_id_t planet_index);
    void TaskArrivedAt(entity_id_t task_index, entity_id_t planet_index);
    void CompleteTask(entity_id_t task_index);

    //int LoadQuests(const DataNode*);
    //int RandomTemplateIndex();
};

#endif  //QUEST_MANAGER_H