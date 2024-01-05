#ifndef QUEST_H
#define QUEST_H

#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "time.hpp"
#include "id_allocator.hpp"
#include "datanode.hpp"
#include "ui.hpp"
#include "task.hpp"
#include "wren_interface.hpp"

struct Ship;

#define QUEST_PANEL_HEIGHT 64
#define DIALOGUE_MAX_ANSWERS 5

struct Quest {
    const WrenQuestTemplate* wren_interface;
    WrenHandle* quest_instance_handle;
    WrenHandle* next_handle;
    WrenHandle* next_result_handle;
    WrenHandle* state_set_handle;
    WrenHandle* serialize_handle;
    WrenHandle* deserialize_handle;

    RID id = GetInvalidId();
    
    // current, await_type and next_options can be regained by re-running the next

    union QuestUnion {
        RID task;
        //Dialogue dialogue;
        timemath::Time wait_until;
        bool quest_sccessfull;

        QuestUnion() {}
    } current;

    enum {
        NOT_STARTED = 0,
        TASK,
        DAILOGUE,
        DAILOGUE_CHOICE,
        WAIT,
        DONE,
    } await_type = NOT_STARTED;
    //std::vector<std::string> next_options;
    char* next_options[DIALOGUE_MAX_ANSWERS] = {
        NULL, NULL, NULL, NULL, NULL
    };

    Quest();
    ~Quest();

    void CopyFrom(Quest* other);
    bool IsValid() const;
    bool IsActive() const;
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;

    void AttachTemplate(const WrenQuestTemplate* p_wren_interface);

    bool StartQuest();
    bool CompleteTask(bool success);
    bool TimePassed();
    bool AnswerDialogue(int choice);   // for both choices and forward

    bool _Activate();
    bool _RunInState(const char* next_state);
    void _NextTask();
};

void ClearQuest(Quest* quest);

#endif  //QUEST_H