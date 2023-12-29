#ifndef QUESTS_H
#define QUESTS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "time.hpp"
#include "id_allocator.hpp"
#include "datanode.hpp"
#include "ui.hpp"
#include "wren_interface.hpp"

struct Ship;

/*struct QuestTemplate {
    std::vector<entity_id_t> departure_options = std::vector<entity_id_t>();
    std::vector<entity_id_t> destination_options = std::vector<entity_id_t>();
    double max_dv;
    double payload;
    cost_t payout;

    entity_id_t GetRandomDeparturePlanet() const;
    entity_id_t GetRandomArrivalPlanet(entity_id_t departure_planet) const;
};*/

struct Task {
    entity_id_t departure_planet;
    entity_id_t arrival_planet;
    entity_id_t current_planet;
    entity_id_t ship;

    entity_id_t quest;

    double payload_mass;
    timemath::Time pickup_expiration_time;
    timemath::Time delivery_expiration_time;
    cost_t payout;

    Task();
    //void CopyFrom(const Task* other);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    bool IsValid() const;
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;
};

struct Quest {
    const WrenQuest* wren_interface;
    WrenHandle* quest_instance_handle;
    WrenHandle* coroutine_instance_handle;
    WrenHandle* coroutine_call_handle;

    entity_id_t id = GetInvalidId();

    Quest();
    ~Quest();

    union QuestUnion {
        entity_id_t task;
        //Dialogue dialogue;
        timemath::Time wait_until;
        bool quest_sccessfull;

        QuestUnion() {}
    } current;

    enum {
        NOT_STARTED,
        TASK,
        DAILOGUE,
        DAILOGUE_CHOICE,
        WAIT,
        DONE,
    } await_type = NOT_STARTED;

    int step = 0;

    void CopyFrom(Quest* other);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;

    void AttachInterface(const WrenQuest* p_wren_interface);

    void StartQuest(int inp_arg=0);  // The way wren is setup, it allows 1 extra arg. Mostly unused tho
    void CompleteTask(bool success);
    void TimePassed();
    void AnswerDialogue(int choice);   // for both choices and forward

    void _NextTask();
};

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

int LoadQuests(const DataNode*);

#endif  //QUESTS_H