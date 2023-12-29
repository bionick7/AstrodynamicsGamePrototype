#include "quest_manager.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

QuestManager::QuestManager() {
    active_tasks.Init();
}

QuestManager::~QuestManager() {
    
}

void QuestManager::Serialize(DataNode* data) const {
    data->SetArrayChild("active_quests", active_tasks.alloc_count);
    for(auto it = active_tasks.GetIter(); it; it++) {
        active_tasks.Get(it)->Serialize(data->SetArrayElemChild("active_quests", it.iterator, DataNode()));
    }
    data->SetArrayChild("available_quests", GetAvailableQuests());
    for(int i=0; i < GetAvailableQuests(); i++) {
        available_quests[i].Serialize(data->SetArrayElemChild("available_quests", i, DataNode()));
    }
}

void QuestManager::Deserialize(const DataNode* data) {
    active_tasks.Clear();
    for(int i=0; i < data->GetArrayChildLen("active_quests"); i++) {
        active_tasks.Get(active_tasks.Allocate())->Deserialize(data->GetArrayChild("active_quests", i));
    }
    for(int i=0; i < data->GetArrayChildLen("available_quests") && i < GetAvailableQuests(); i++) {
        available_quests[i].Deserialize(data->GetArrayChild("available_quests", i));
    }
    /*for(int i=data->GetArrayChildLen("available_quests"); i < GetAvailableQuests(); i++) {
        available_quests[i] = Quest();  // Just in case
    }*/
}

void QuestManager::Make() {
    for(int i=0; i < GetAvailableQuests(); i++) {
        const WrenQuest* template_ = GetWrenInterface()->GetRandomWrenQuest();
        available_quests[i].AttachInterface(template_);
    }
}

void QuestManager::Update(double dt) {
    timemath::Time now = GlobalGetNow();

    if (IsKeyPressed(KEY_Q)) show_ui = !show_ui;
    
    // Remove all expired and completed quests
    for(auto i = active_quests.GetIter(); i; i++) {
        switch (active_quests[i]->await_type) {
            case Quest::TASK: {
                entity_id_t task_id = active_quests[i]->current.task;
                Task* task = active_tasks[task_id];
                bool is_in_transit = IsIdValid(task->ship) && !GetShip(task->ship)->is_parked;
                if (task->pickup_expiration_time < now && !is_in_transit) {
                    active_quests[i]->CompleteTask(false);
                    active_tasks.Erase(task_id);
                }
                else if (task->delivery_expiration_time < now) {
                    active_quests[i]->CompleteTask(false);
                    active_tasks.Erase(task_id);
                }
                break; }
            case Quest::WAIT: {
                if (active_quests[i]->current.wait_until < now) {
                    active_quests[i]->TimePassed();
                }
                break;}
            default: 
                INFO("%d", active_quests[i]->await_type)
                NOT_IMPLEMENTED
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

    if (GlobalGetState()->calendar.IsNewDay()) {
        for(int i=0; i < GetAvailableQuests(); i++) {
            const WrenQuest* template_ = GetWrenInterface()->GetRandomWrenQuest();
            available_quests[i].AttachInterface(template_);
        }
    }
}

int current_available_quests_scroll = 0;
void QuestManager::Draw() {
    if (!show_ui) return;
    
    int x_margin = MinInt(100, GetScreenWidth()*.1);
    int y_margin = MinInt(50, GetScreenWidth()*.1);
    int w = GetScreenWidth() - x_margin*2;
    int h = GetScreenHeight() - y_margin*2;
    UIContextCreate(x_margin, y_margin, w, h, 16, Palette::ui_main);
    UIContextEnclose(Palette::bg, Palette::ui_main);
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

    if (GlobalGetState()->current_focus == GlobalState::QUEST_MANAGER) {
        int max_scroll = MaxInt(TASK_PANEL_HEIGHT * GetAvailableQuests() - UIContextCurrent().height, 0);
        current_available_quests_scroll = ClampInt(current_available_quests_scroll - GetMouseWheelMove() * 20, 0, max_scroll);
    }

    UIContextPushScrollInset(0, UIContextCurrent().height, TASK_PANEL_HEIGHT * GetAvailableQuests(), current_available_quests_scroll);
    // Available Quests
    for(int i=0; i < GetAvailableQuests(); i++) {
        if (!available_quests[i].IsValid()) continue;
        if (available_quests[i].DrawUI(true, true) & BUTTON_STATE_FLAG_JUST_PRESSED) {
            AcceptQuest(i);
        }
    }
    UIContextPop();  // ScrollInseet

    UIContextPop();  // HSplit
}

void QuestManager::AcceptQuest(entity_id_t quest_index) {
    Quest* q;
    entity_id_t id = active_quests.Allocate(&q);
    q->CopyFrom(&available_quests[quest_index]);
    q->id = id;
    q->StartQuest();
    ClearQuest(&available_quests[quest_index]);
}

entity_id_t QuestManager::CreateTask(entity_id_t quest_index) {
    Task* task;
    entity_id_t id = active_tasks.Allocate(&task);
    task->quest = quest_index;
    return id;
}

void QuestManager::PickupTask(entity_id_t ship_index, entity_id_t task_index)
{
    active_tasks[task_index]->ship = ship_index;
    //ship->payload.push_back(TransportContainer(quest_index));
}

void QuestManager::PutbackTask(entity_id_t ship_index, entity_id_t task_index) {
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
    if (!ship->is_parked) {
        ERROR("'%s' must be parked on planet to deliver quest", task_index, ship->name)
        return;
    }
    active_tasks[task_index]->ship = GetInvalidId();
    //ship->payload.erase(quest_in_cargo);
}

void QuestManager::TaskDepartedFrom(entity_id_t task_index, entity_id_t planet_index) {
    active_tasks[task_index]->current_planet = GetInvalidId();
}

void QuestManager::TaskArrivedAt(entity_id_t task_index, entity_id_t planet_index) {
    Task* q = active_tasks[task_index];
    q->current_planet = planet_index;
    if (q->arrival_planet == planet_index) {
        CompleteTask(task_index);
    }
}

void QuestManager::CompleteTask(entity_id_t task_index) {
    Task* q = active_tasks[task_index];
    INFO("Task completed (M§M %f)", q->payout)
    //GlobalGetState()->CompleteTransaction(q->payout, "Completed quest");
    active_quests[q->quest]->CompleteTask(true);
    active_tasks.Erase(task_index);
}

int QuestManager::GetAvailableQuests() const {
    return _AVAILABLE_QUESTS;
}
