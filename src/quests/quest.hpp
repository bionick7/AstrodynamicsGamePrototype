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

struct Quest {
    const WrenQuestTemplate* wren_interface;
    WrenHandle* quest_instance_handle;
    WrenHandle* coroutine_instance_handle;
    WrenHandle* coroutine_call_handle;

    RID id = GetInvalidId();

    Quest();
    ~Quest();

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

    int step = 0;

    void CopyFrom(Quest* other);
    bool IsValid() const;
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    ButtonStateFlags DrawUI(bool show_as_button, bool highlight) const;

    void AttachTemplate(const WrenQuestTemplate* p_wren_interface);

    void StartQuest(int inp_arg=0);  // The way wren is setup, it allows 1 extra arg. Mostly unused tho
    void CompleteTask(bool success);
    void TimePassed();
    void AnswerDialogue(int choice);   // for both choices and forward

    void _NextTask();
};

void ClearQuest(Quest* quest);

#endif  //QUEST_H