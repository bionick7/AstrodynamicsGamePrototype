#include "quests.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

const int QUEST_PANEL_HEIGHT = 64;

void _ClearQuest(Quest* quest) {
    quest->wren_interface = NULL;
    quest->quest_instance_handle = NULL;
    quest->coroutine_instance_handle = NULL;
    quest->coroutine_call_handle = NULL;

    quest->id = GetInvalidId();

    quest->await_type = Quest::NOT_STARTED;
    quest->step = 0;
}

Quest::Quest() {
    _ClearQuest(this);
}

void Quest::AttachInterface(const WrenQuest* p_wren_interface) {
    wren_interface = p_wren_interface;
}

void Quest::StartQuest(int inp_arg) {
    WrenVM* vm = GetWrenVM();
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, wren_interface->class_handle);
    WrenHandle* constructor_handle = wrenMakeCallHandle(vm, "new()");
    wrenCall(vm, constructor_handle);
    quest_instance_handle = wrenGetSlotHandle(vm, 0);
    WrenHandle* mainfunc_callhandle = wrenMakeCallHandle(vm, "main");
    // instance handle still in slot 0
    wrenCall(vm, mainfunc_callhandle);
    coroutine_instance_handle = wrenGetSlotHandle(vm, 0);
    // coroutine still in slot 0
    coroutine_call_handle = wrenMakeCallHandle(vm, "call(_)");

    wrenReleaseHandle(vm, constructor_handle);
    wrenReleaseHandle(vm, mainfunc_callhandle);

    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, coroutine_instance_handle);
    wrenSetSlotDouble(vm, 1, (double) inp_arg);
    GetWrenInterface()->CallFunc(coroutine_call_handle);
    _NextTask();
}

void Quest::CompleteTask(bool success) {
    if (await_type != TASK) {
        return;
    }
    WrenVM* vm = GetWrenVM();
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, coroutine_instance_handle);
    wrenSetSlotBool(vm, 1, success);
    GetWrenInterface()->CallFunc(coroutine_call_handle);
    _NextTask();
}

void Quest::AnswerDialogue(int choice) {
    if (await_type != DAILOGUE && await_type != DAILOGUE_CHOICE) {
        return;
    }
    WrenVM* vm = GetWrenVM();
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, coroutine_instance_handle);
    wrenSetSlotDouble(vm, 1, (double) choice);
    GetWrenInterface()->CallFunc(coroutine_call_handle);
    _NextTask();
}

void Quest::TimePassed() {
    if (await_type != WAIT) {
        return;
    }
    WrenVM* vm = GetWrenVM();
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, coroutine_instance_handle);
    wrenSetSlotNull(vm, 1);
    GetWrenInterface()->CallFunc(coroutine_call_handle);
    _NextTask();
}

void Quest::_NextTask() {
    WrenVM* vm = GetWrenVM();
    WrenType return_type = wrenGetSlotType(vm, 0);
    await_type = DONE;  // fallback
    if (return_type == WREN_TYPE_BOOL) {
        await_type = DONE;  // explicitly
        bool quest_success = wrenGetSlotBool(vm, 0);
        return;
    }
    if (return_type != WREN_TYPE_MAP) {
        return;
    }

    const char* type = GetWrenInterface()->GetStringFromMap("type", "invalid");
    if (strcmp(type, "task") == 0) {
        await_type = TASK;
        current.task = GlobalGetState()->quest_manager.CreateTask(id);
        Task* task = GlobalGetState()->quest_manager.active_tasks.Get(current.task);

        task->departure_planet = (int) GetWrenInterface()->GetNumFromMap("departure_planet", 0);
        task->current_planet = (int) GetWrenInterface()->GetNumFromMap("departure_planet", 0);
        task->arrival_planet = (int) GetWrenInterface()->GetNumFromMap("arrival_planet", 0);
        task->payload_mass = GetWrenInterface()->GetNumFromMap("payload_mass", 0);
        task->pickup_expiration_time = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("departure_time_offset", 0));
        task->delivery_expiration_time = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("arrival_time_offset", 0));
    }
    else if (strcmp(type, "wait") == 0) {
        await_type = WAIT;
        current.wait_until = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("wait_time", 0));
    }
    else if (strcmp(type, "dialogue") == 0) {
        await_type = DAILOGUE;
        NOT_IMPLEMENTED
    }
    else if (strcmp(type, "dialogue choice") == 0) {
        await_type = DAILOGUE_CHOICE;
        NOT_IMPLEMENTED
    }
}

Quest::~Quest() {
    WrenVM* vm = GetWrenVM();
    if (quest_instance_handle != NULL) wrenReleaseHandle(vm, quest_instance_handle);
    if (coroutine_call_handle != NULL) wrenReleaseHandle(vm, coroutine_call_handle);
}

void Quest::Serialize(DataNode* data) const {
    NOT_IMPLEMENTED
}

void Quest::Deserialize(const DataNode* data) {
    NOT_IMPLEMENTED
}

ButtonStateFlags Quest::DrawUI(bool show_as_button, bool highlight) const {
    int height = UIContextPushInset(3, QUEST_PANEL_HEIGHT);

    if (height == 0) {
        UIContextPop();
        return BUTTON_STATE_FLAG_NONE;
    }
    if (highlight) {
        UIContextEnclose(Palette::bg, Palette::ship);
    } else {
        UIContextEnclose(Palette::bg, Palette::ui_main);
    }
    UIContextShrink(6, 6);
    ButtonStateFlags button_state = UIContextAsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    }
    if (height != QUEST_PANEL_HEIGHT) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    UIContextPop();
    return button_state;
}

void Quest::CopyFrom(Quest* other) {
    wren_interface = other->wren_interface;
    current = other->current;
    await_type = other->await_type;
    step = other->step;
}


// ========================================
//              Quest Template
// ========================================

/*
entity_id_t QuestTemplate::GetRandomDeparturePlanet() const {
    int selector = GetRandomValue(0, departure_options.size() - 1);
    return departure_options[selector];
}

entity_id_t QuestTemplate::GetRandomArrivalPlanet(entity_id_t departure_planet) const {
    entity_id_t destination_planet;
    double dv1, dv2;
    int iter_count = 0;
    do {
        int selector = GetRandomValue(0, destination_options.size() - 1);
        destination_planet = destination_options[selector];
        HohmannTransfer(
            &GetPlanet(departure_planet)->orbit,
            &GetPlanet(destination_planet)->orbit,
            GlobalGetNow(),
            NULL, NULL,
            &dv1, &dv2
        );
        if (iter_count++ > 1000) {
            ERROR("Quest Generation: Arrival planet finding exceeds iteration Count (>1000)")
            INFO("%f + %f ><? %f", dv1, dv2, max_dv)
            return departure_planet;  // Better broken quest than infinite freeze
        }
    } while (
        destination_planet == departure_planet || dv1 + dv2 > max_dv
    );
    //INFO("%d => %d :: %f + %f < %f", departure_planet, destination_planet, dv1, dv2, max_dv)
    return destination_planet;
}
*/

// ========================================
//                  Quest
// ========================================

void _ClearTask(Task* quest) {
    quest->departure_planet = GetInvalidId();
    quest->arrival_planet = GetInvalidId();
    quest->current_planet = GetInvalidId();
    quest->ship = GetInvalidId();
    quest->quest = GetInvalidId();

    quest->payload_mass = 0;
    quest->pickup_expiration_time = timemath::Time::GetInvalid();
    quest->delivery_expiration_time = timemath::Time::GetInvalid();

    quest->payout = 0;
}

Task::Task() {
    _ClearTask(this);
}

/*void Task::CopyFrom(const Task* other) {
    departure_planet = other->departure_planet;
    arrival_planet = other->arrival_planet;
    current_planet = other->current_planet;
    ship = other->ship;

    payload_mass = other->payload_mass;
    pickup_expiration_time = other->pickup_expiration_time;
    delivery_expiration_time = other->delivery_expiration_time;

    payout = other->payout;
}*/

void Task::Serialize(DataNode* data) const {
    data->SetI("departure_planet", departure_planet);
    data->SetI("arrival_planet", arrival_planet);
    data->SetI("current_planet", current_planet);
    data->SetI("ship", ship);
    data->SetI("quest", quest);

    data->SetF("payload_mass", payload_mass);
    data->SetDate("pickup_expiration_time", pickup_expiration_time);
    data->SetDate("delivery_expiration_time", delivery_expiration_time);
    data->SetF("payout", payout);
}

void Task::Deserialize(const DataNode* data) {
    departure_planet =          data->GetI("departure_planet", departure_planet);
    arrival_planet =            data->GetI("arrival_planet", arrival_planet);
    current_planet =            data->GetI("current_planet", current_planet);
    ship =                      data->GetI("ship", ship);
    quest =                     data->GetI("quest", quest);
    payload_mass =              data->GetF("payload_mass", payload_mass);
    pickup_expiration_time =    data->GetDate("pickup_expiration_time", pickup_expiration_time);
    delivery_expiration_time =  data->GetDate("delivery_expiration_time", delivery_expiration_time);
    payout =                    data->GetF("payout", payout);
}

bool Task::IsValid() const {
    return IsIdValid(departure_planet) && IsIdValid(arrival_planet);
}

ButtonStateFlags Task::DrawUI(bool show_as_button, bool highlight) const {
    // Assumes parent UI Context exists
    // Resturns if player wants to accept
    if (!IsValid()) {
        return BUTTON_STATE_FLAG_NONE;
    }

    int height = UIContextPushInset(3, QUEST_PANEL_HEIGHT);

    if (height == 0) {
        UIContextPop();
        return BUTTON_STATE_FLAG_NONE;
    }
    if (highlight) {
        UIContextEnclose(Palette::bg, Palette::ship);
    } else {
        UIContextEnclose(Palette::bg, Palette::ui_main);
    }
    UIContextShrink(6, 6);
    ButtonStateFlags button_state = UIContextAsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    }
    if (height != QUEST_PANEL_HEIGHT) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    StringBuilder sb_mouse;
    double dv1, dv2;
    HohmannTransfer(&GetPlanet(departure_planet)->orbit, &GetPlanet(arrival_planet)->orbit, GlobalGetNow(), NULL, NULL, &dv1, &dv2);
    sb_mouse.AddFormat("DV: %.3f km/s\n", (dv1 + dv2) / 1000);

    StringBuilder sb;
    // Line 1
    sb.AddFormat("%s >> %s  %d cts", GetPlanet(departure_planet)->name, GetPlanet(arrival_planet)->name, KGToResourceCounts(payload_mass));
    if (button_state == BUTTON_STATE_FLAG_HOVER) {
        UISetMouseHint(sb_mouse.c_str);
    }
    if (IsIdValid(current_planet)) {
        sb.Add("  Now: [").Add(GetPlanet(current_planet)->name).AddLine("]");
    } else sb.AddLine("");
    // Line 2
    bool is_in_transit = IsIdValid(ship) && !GetShip(ship)->is_parked;
    if (is_in_transit) {
        sb.Add("Expires in ").AddTime(delivery_expiration_time - GlobalGetNow());
    } else {
        sb.Add("Expires in ").AddTime(pickup_expiration_time - GlobalGetNow());
    }
    sb.AddFormat("  => ").AddCost(payout);
    UIContextWrite(sb.c_str);

    UIContextPop();
    return button_state;
}

// ========================================
//              QuestManager
// ========================================

QuestManager::QuestManager() {
    active_tasks.Init();
}

QuestManager::~QuestManager() {
    //delete[] templates;
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
        int max_scroll = MaxInt(QUEST_PANEL_HEIGHT * GetAvailableQuests() - UIContextCurrent().height, 0);
        current_available_quests_scroll = ClampInt(current_available_quests_scroll - GetMouseWheelMove() * 20, 0, max_scroll);
    }

    UIContextPushScrollInset(0, UIContextCurrent().height, QUEST_PANEL_HEIGHT * GetAvailableQuests(), current_available_quests_scroll);
    // Available Quests
    for(int i=0; i < GetAvailableQuests(); i++) {
        if(available_quests[i].DrawUI(true, true) & BUTTON_STATE_FLAG_JUST_PRESSED) {
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
    _ClearQuest(&available_quests[quest_index]);
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

/*int QuestManager::LoadQuests(const DataNode* data) {
    delete[] templates;

    template_count = data->GetArrayChildLen("resource_missions");
    templates = new QuestTemplate[template_count];
    for(int i=0; i < template_count; i++) {
        const DataNode* mission_data = data->GetArrayChild("resource_missions", i);

        templates[i].payload = mission_data->GetF("payload", 0.0) * 1000;
        mission_data->Get("flavour", "UNFLAVOURED");
        for (int j=0; j < mission_data->GetArrayLen("departure"); j++) {
            entity_id_t planet = GlobalGetState()->planets.GetIndexByName(mission_data->GetArray("departure", j));
            templates[i].departure_options.push_back(planet);
        }

        for (int j=0; j < mission_data->GetArrayLen("destination"); j++) {
            entity_id_t planet = GlobalGetState()->planets.GetIndexByName(mission_data->GetArray("destination", j));
            templates[i].destination_options.push_back(planet);
        }
        templates[i].max_dv = INFINITY;
        for (int j=0; j < mission_data->GetArrayLen("ship_accesibilty"); j++) {
            entity_id_t ship_class = GlobalGetState()->ships.GetShipClassIndexById(mission_data->GetArray("ship_accesibilty", j));
            const ShipClass* sc = GetShipClassByIndex(ship_class);
            double max_dv_class = sc->v_e * log(ResourceCountsToKG(sc->max_capacity) + sc->oem) - sc->v_e * log(templates[i].payload + sc->oem);
            if (max_dv_class < templates[i].max_dv) templates[i].max_dv = max_dv_class;
        }

        // TODO: Check if quest is possible for each startere planet

        templates[i].payout = (int) mission_data->GetF("payout", 0.0);  // to allow for exponential notation etc.
    }
    return template_count;
}*/

// ========================================
//                  General
// ========================================

int LoadQuests(const DataNode* data) {
    //return GlobalGetState()->quest_manager.LoadQuests(data);
    return 0;
}