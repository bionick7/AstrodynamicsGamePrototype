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

struct QuestManager {
    IDAllocatorList<Quest, EntityType::QUEST> available_quests;
    IDAllocatorList<Quest, EntityType::ACTIVE_QUEST> active_quests;
    IDAllocatorList<Task, EntityType::TASK> active_tasks;
    IDAllocatorList<Dialogue, EntityType::DIALOGUE> dialogues;

    bool show_ui;
    
    QuestManager();
    ~QuestManager();
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void Make();
    void Update(double dt);
    void Draw();

    void AcceptQuest(RID quest_index);
    void ForceQuest(WrenQuestTemplate* template_);
    RID CreateTask(RID quest_index);
    int GetAvailableQuests() const;

    void PickupTask(RID ship_index, RID task_index);
    void PutbackTask(RID ship_index, RID task_index);
    void TaskDepartedFrom(RID task_index, RID planet_index);
    void TaskArrivedAt(RID task_index, RID planet_index);
    void CompleteTask(RID task_index);

    RID CreateDialogue(const char* speaker, const char* body, const char* replies[], int reply_count);
    const Dialogue* GetDialogue(RID dielogue_index) const;
    void EraseDialogue(RID dielogue_index);

    void _RegenQuests();

    //int LoadQuests(const DataNode*);
    //int RandomTemplateIndex();
};

#endif  //QUEST_MANAGER_H