#include "quest_manager.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

QuestManager::QuestManager() {

}

QuestManager::~QuestManager() {
    
}

void QuestManager::Serialize(DataNode* data) const {
    dialogues.SerializeInto(data, "dialogues", [](DataNode* dn, const Dialogue* d){ d->Serialize(dn); });
    available_quests.SerializeInto(data, "available_quests", [](DataNode* dn, const Quest* q){ q->Serialize(dn); });
    active_quests.SerializeInto(data, "active_quests", [](DataNode* dn, const Quest* q){ q->Serialize(dn); });
}

void QuestManager::Deserialize(const DataNode* data) {
    // Order matters!
    active_tasks.Clear();
    dialogues.DeserializeFrom(data, "dialogues", [](const DataNode* dn, Dialogue* d){ d->Deserialize(dn); });
    available_quests.DeserializeFrom(data, "available_quests", [](const DataNode* dn, Quest* q){ q->Deserialize(dn); });
    active_quests.DeserializeFrom(data, "active_quests", [](const DataNode* dn, Quest* q){ q->Deserialize(dn); });
}

void QuestManager::Make() {
    _RegenQuests();
}

void QuestManager::Update(double dt) {
    timemath::Time now = GlobalGetNow();

    if (IsKeyPressed(KEY_Q)) show_ui = !show_ui;
    
    // Remove all expired and completed quests
    for(auto it = active_quests.GetIter(); it; it++) {
        switch (active_quests[it]->await_type) {
            case Quest::TASK: {
                RID task_id = active_quests[it]->current.task;
                Task* task = active_tasks[task_id];
                bool is_in_transit = IsIdValid(task->ship) && !GetShip(task->ship)->IsParked();
                if (task->pickup_expiration_time < now && !is_in_transit) {
                    bool success = active_quests[it]->CompleteTask(false);
                    if (!success) active_quests.Erase(it.GetId());
                    active_tasks.Erase(task_id);
                }
                else if (task->delivery_expiration_time < now) {
                    bool success = active_quests[it]->CompleteTask(false);;
                    if (!success) active_quests.Erase(it.GetId());
                    active_tasks.Erase(task_id);
                }
                break; }
            case Quest::WAIT: {
                if (active_quests[it]->current.wait_until < now) {
                    bool success = active_quests[it]->TimePassed();
                    if (!success) active_quests.Erase(it.GetId());
                }
                break;}
            case Quest::DONE:{
                active_quests.Erase(it.GetId());
                break;}
            case Quest::DAILOGUE:{
                const Dialogue* dialogue = GetDialogue(active_quests[it]->current.dialogue);
                if (dialogue->reply >= 0) {
                    bool success = active_quests[it]->AnswerDialogue(dialogue->reply);
                    if (!success) active_quests.Erase(it.GetId());
                }
                break;}
            default: 
                INFO("%d", active_quests[it]->await_type)
                NOT_REACHABLE
        }
    }
    /*for(auto i = active_tasks.GetIter(); i; i++) {
        Task* task = active_tasks[i];
        bool is_in_transit = IsIdValid(task->ship) && !GetShip(task->ship)->is_parked;
        if (task->pickup_expiration_time < now && !is_in_transit) {
            active_tasks.Erase(i);
        }
        if (task->delivery_expiration_time < now) {
            active_tasks.Erase(i);
        }
    }*/
    /*for(int i=0; i < GetAvailableQuests(); i++) {
        if (available_quests[i].pickup_expiration_time < now) {
            _GenerateRandomQuest(&available_quests[i], &templates[RandomTemplateIndex()]);
        }
    }*/

    if (GetCalendar()->IsNewDay()) {
        _RegenQuests();
    }
}

int current_available_quests_scroll = 0;
int current_tab_qst;
void QuestManager::Draw() {
    if (!show_ui) return;
    
    int x_margin = MinInt(100, GetScreenWidth()*.1);
    int y_margin = MinInt(50, GetScreenWidth()*.1);
    int w = GetScreenWidth() - x_margin*2;
    int h = GetScreenHeight() - y_margin*2;
    UIContextCreateNew(x_margin, y_margin, w, h, 16, Palette::ui_main);
    UIContextEnclose(Palette::bg, Palette::ui_main);

    // TABS

    UIContextPushInset(4, 20);  // Tab container
    const int n_tabs = active_quests.Count() + 1;
    if (current_tab_qst >= n_tabs) {
        current_tab_qst = 0;
    }
    auto it = active_quests.GetIter();
    int tab_width = w / n_tabs;
    if (tab_width > 150) tab_width = 150;
    for (int i_tab = 0; i_tab < n_tabs; i_tab++) {
        UIContextPushHSplit(i_tab * tab_width, (i_tab + 1) * tab_width);
        ButtonStateFlags::T button_state = UIContextAsButton();
        HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            current_tab_qst = i_tab;
        }
        if (button_state & ButtonStateFlags::HOVER || i_tab == current_tab_qst) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        if (i_tab == 0) {
            UIContextWrite("tasks");
        } else {
            UIContextWrite(active_quests[it]->wren_interface->id);
            it++;
        }
        UIContextPop();  // HSplit
    }
    UIContextPop();  // Tab container

    UIContextPushInset(0, h - 20);
    if (current_tab_qst == 0) {
        UIContextPushHSplit(0, w/2);
        UIContextShrink(5, 5);
        // Active quests
        if (active_tasks.Count() == 0) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        for(auto i = active_tasks.GetIter(); i; i++) {
            active_tasks[i]->DrawUI(false, IsIdValid(active_tasks[i]->ship));
        }
        UIContextPop();  // HSplit

        UIContextPushHSplit(w/2, w);
        UIContextShrink(5, 5);

        if (GetAvailableQuests() == 0) {
            UIContextPop();  // HSplit
            return;
        }

        if (GetGlobalState()->current_focus == GlobalState::QUEST_MANAGER) {
            int max_scroll = MaxInt(TASK_PANEL_HEIGHT * GetAvailableQuests() - UIContextCurrent().height, 0);
            current_available_quests_scroll = ClampInt(current_available_quests_scroll - GetMouseWheelMove() * 20, 0, max_scroll);
        }

        UIContextPushScrollInset(0, UIContextCurrent().height, TASK_PANEL_HEIGHT * GetAvailableQuests(), &current_available_quests_scroll);
        // Available Quests
        for(auto it = available_quests.GetIter(); it; it++) {
            if (!available_quests[it]->IsValid()) continue;
            if (available_quests[it]->DrawUI(true, true) & ButtonStateFlags::JUST_PRESSED) {
                AcceptQuest(it.GetId());
            }
        }
        UIContextPop();  // ScrollInseet
        UIContextPop();  // HSplit
    } else {
        auto it = active_quests.GetIter();
        for(int i=1; i < current_tab_qst; i++) it++;
        Quest* q = active_quests[it];
        for(int i=0; i < q->dialogue_backlog.size; i++) {
            UIContextPushInset(0, 100);
            dialogues[q->dialogue_backlog.Get(i)]->DrawToUIContext();
            UIContextPop();  // Inset
        }
        if (active_quests[it]->await_type == Quest::DAILOGUE) {
            UIContextPushInset(0, 100);
            dialogues[active_quests[it]->current.dialogue]->DrawToUIContext();
            UIContextPop();  // Inset
        }
        active_quests[it];
    }
    UIContextPop();  // Inset
}

void QuestManager::AcceptQuest(RID quest_index) {
    Quest* q;
    RID id = active_quests.Allocate(&q);
    q->CopyFrom(available_quests[quest_index]);
    q->id = id;
    bool success = q->StartQuest();
    if (!success) active_quests.Erase(id);
    available_quests.Erase(quest_index);
}

void QuestManager::ForceQuest(const WrenQuestTemplate *template_) {
    Quest* q;
    RID id = active_quests.Allocate(&q);
    q->AttachTemplate(template_);
    q->id = id;
    bool success = q->StartQuest();
    if (!success) active_quests.Erase(id);
}

RID QuestManager::CreateTask(RID quest_index) {
    Task* task;
    RID id = active_tasks.Allocate(&task);
    task->quest = quest_index;
    return id;
}

void QuestManager::PickupTask(RID ship_index, RID task_index)
{
    active_tasks[task_index]->ship = ship_index;
    //ship->payload.push_back(TransportContainer(quest_index));
}

void QuestManager::PutbackTask(RID ship_index, RID task_index) {
    Ship* ship = GetShip(ship_index);
    //auto quest_in_cargo = ship->payload.end();
    //for(auto it2=ship->payload.begin(); it2 != ship->payload.end(); it2++) {
    //    if (it2->type == TransportContainer::QUEST && it2->content.quest == quest_index) {
    //        quest_in_cargo = it2;
    //    }
    //}
    if (active_tasks[task_index]->ship != ship_index) {
        ERROR("Quest %d not currently on ship '%s'", task_index, ship->name)
        return;
    }
    if (!ship->IsParked()) {
        ERROR("'%s' must be parked on planet to deliver quest", task_index, ship->name)
        return;
    }
    active_tasks[task_index]->ship = GetInvalidId();
    //ship->payload.erase(quest_in_cargo);
}

void QuestManager::TaskDepartedFrom(RID task_index, RID planet_index) {
    active_tasks[task_index]->current_planet = GetInvalidId();
}

void QuestManager::TaskArrivedAt(RID task_index, RID planet_index) {
    Task* q = active_tasks[task_index];
    q->current_planet = planet_index;
    if (q->arrival_planet == planet_index) {
        CompleteTask(task_index);
    }
}

void QuestManager::CompleteTask(RID task_index) {
    Task* q = active_tasks[task_index];
    INFO("Task completed (M§M %f)", q->payout)
    active_quests[q->quest]->CompleteTask(true);
    active_tasks.Erase(task_index);
}

int QuestManager::GetAvailableQuests() const {
    return available_quests.Count();
}

RID QuestManager::CreateDialogue(const char* speaker, const char* body, const char* replies[], int reply_count) {
    Dialogue* d;
    RID res = dialogues.Allocate(&d);
    d->Setup(speaker, body, replies, reply_count);
    return res;
}

const Dialogue* QuestManager::GetDialogue(RID dialogue_index) const {
    return dialogues.Get(dialogue_index);
}

void QuestManager::EraseDialogue(RID dialogue_index) {
    dialogues.Erase(dialogue_index);
}


void QuestManager::_RegenQuests() {
    int available = GetRandomValue(0, 4);
    available_quests.Init();
    for(int i=0; i < available; i++) {
        const WrenQuestTemplate* template_ = GetWrenInterface()->GetRandomWrenQuest();
        Quest* quest;
        available_quests.Allocate(&quest);
        quest->AttachTemplate(template_);
    }   
}