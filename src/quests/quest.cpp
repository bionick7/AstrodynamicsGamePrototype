#include "quest.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

void ClearQuest(Quest* quest) {
    quest->wren_interface = NULL;
    quest->quest_instance_handle = NULL;
    quest->next_handle = NULL;
    quest->next_result_handle = NULL;
    quest->state_set_handle = NULL;
    quest->serialize_handle = NULL;
    quest->deserialize_handle = NULL;

    quest->id = GetInvalidId();
    quest->await_type = Quest::NOT_STARTED;
    for (int i=0; i < DIALOGUE_MAX_REPLIES; i++) {
        delete[] quest->next_options[i];
        quest->next_options[i] = NULL;
    }

    quest->dialogue_backlog.Clear();
}

Quest::Quest() {
    for (int i=0; i < DIALOGUE_MAX_REPLIES; i++) {
        next_options[i] = NULL;
    }
    ClearQuest(this);
}

Quest::~Quest() {
    WrenVM* vm = GetWrenVM();
    if (quest_instance_handle != NULL) wrenReleaseHandle(vm, quest_instance_handle);
    if (next_handle != NULL) wrenReleaseHandle(vm, next_handle);
    if (next_result_handle != NULL) wrenReleaseHandle(vm, next_result_handle);
    if (state_set_handle != NULL) wrenReleaseHandle(vm, state_set_handle);
    if (serialize_handle != NULL) wrenReleaseHandle(vm, serialize_handle);
    if (deserialize_handle != NULL) wrenReleaseHandle(vm, deserialize_handle);
}

void Quest::CopyFrom(Quest* other) {
    wren_interface = other->wren_interface;
    current = other->current;
    await_type = other->await_type;
}

bool Quest::IsValid() const {
    return wren_interface != NULL;
}

bool Quest::IsActive() const {
    return 
        IsValid()
        && quest_instance_handle != NULL
        && next_handle != NULL
        && next_result_handle != NULL
        && state_set_handle != NULL
        && serialize_handle != NULL
        && deserialize_handle != NULL
    ;
}

void Quest::Serialize(DataNode* data) const {
    if (!IsValid()) 
        return;
    data->Set("template", wren_interface->id); 
    data->SetI("await_type", await_type); 
    bool active = IsActive();
    data->Set("active", active ? "y": "n");
    if (!active) 
        return;
    WrenVM* vm = GetWrenInterface()->vm;
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    bool success = GetWrenInterface()->CallFunc(serialize_handle);
    if (success) {
        GetWrenInterface()->MapAsDataNode(data);
    }
}

void Quest::Deserialize(const DataNode* data) {
    AttachTemplate(GetWrenInterface()->GetWrenQuest(data->Get("template")));
    if (strcmp(data->Get("active", "n"), "y") != 0) {
        return;
    }
    if(!_Activate()) return;  // TODO: delete?
    WrenVM* vm = GetWrenInterface()->vm;
    GetWrenInterface()->DataNodeToMap(data);
    wrenEnsureSlots(vm, 2);
    GetWrenInterface()->MoveSlot(0, 1);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    GetWrenInterface()->CallFunc(deserialize_handle);

    
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    if(!GetWrenInterface()->CallFunc(next_handle)) return;
    _NextTask();
}

ButtonStateFlags::T Quest::DrawUI(bool show_as_button, bool highlight) const {
    int height = ui::PushInset(3, QUEST_PANEL_HEIGHT);

    if (height == 0) {
        ui::Pop();
        return ButtonStateFlags::NONE;
    }
    if (highlight) {
        ui::EncloseEx(4, Palette::bg, Palette::ally, 4);
    } else {
        ui::Enclose();
    }
    ui::Shrink(6, 6);
    ButtonStateFlags::T button_state = ui::AsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (ButtonStateFlags::JUST_HOVER_IN | ButtonStateFlags::JUST_PRESSED));
    }
    if (height != QUEST_PANEL_HEIGHT) {
        ui::Pop();
        return button_state & ButtonStateFlags::JUST_PRESSED;
    }

    ui::Pop();
    return button_state;
}

void Quest::AttachTemplate(const WrenQuestTemplate* p_wren_interface) {
    wren_interface = p_wren_interface;
}

bool Quest::StartQuest() {
    WrenVM* vm = GetWrenVM();
    if(!_Activate()) return false;
    
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    if(!GetWrenInterface()->CallFunc(next_handle)) return false;
    
    _NextTask();
    return true;
}

bool Quest::CompleteTask(bool success) {
    if (await_type != TASK) return false;
    return _RunInState(next_options[success ? 0 : 1]);
}

bool Quest::TimePassed() {
    if (await_type != WAIT) return false;
    return _RunInState(next_options[0]);
}

bool Quest::AnswerDialogue(int choice) {
    if (await_type != DAILOGUE) return false;
    dialogue_backlog.Append(current.dialogue);
    return _RunInState(next_options[choice]);
}

bool Quest::_Activate() {
    WrenVM* vm = GetWrenVM();
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, wren_interface->class_handle);
    WrenHandle* constructor_handle = wrenMakeCallHandle(vm, "new()");
    if(!GetWrenInterface()->CallFunc(constructor_handle)) {
        wrenReleaseHandle(vm, constructor_handle);
        return false;
    }
    wrenReleaseHandle(vm, constructor_handle);
    quest_instance_handle = wrenGetSlotHandle(vm, 0);
    next_handle = wrenMakeCallHandle(vm, "next()");
    next_result_handle = wrenMakeCallHandle(vm, "next_result()");
    state_set_handle = wrenMakeCallHandle(vm, "state=(_)");
    serialize_handle = wrenMakeCallHandle(vm, "serialize()");
    deserialize_handle = wrenMakeCallHandle(vm, "deserialize(_)");
    return true;
}

bool Quest::_RunInState(const char *next_state) {
    WrenVM* vm = GetWrenVM();
    if (strcmp(next_state, "endS") == 0) {
        await_type = DONE;  // explicitly
        return true;
    }
    if (strcmp(next_state, "endF") == 0) {
        await_type = DONE;  // explicitly
        return true;
    }

    // call goto on the result
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    wrenSetSlotString(vm, 1, next_state);
    if(!GetWrenInterface()->CallFunc(state_set_handle)) return false;

    // call next_handle
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    if(!GetWrenInterface()->CallFunc(next_handle)) return false;

    _NextTask();
    return true;
}

void Quest::_NextTask() {
    WrenVM* vm = GetWrenVM();
    WrenType return_type = wrenGetSlotType(vm, 0);
    await_type = DONE;  // fallback
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, quest_instance_handle);
    if(!GetWrenInterface()->CallFunc(next_result_handle)) return;
    
    WrenType next_result_type = wrenGetSlotType(vm, 0);
    while (next_result_type == WREN_TYPE_MAP) {
        const char* type = GetWrenInterface()->GetStringFromMap("type", "invalid");
        if (strcmp(type, "task") == 0) {
            await_type = TASK;
            current.task = GetQuestManager()->CreateTask(id);
            Task* task = GetQuestManager()->active_tasks.Get(current.task);

            task->departure_planet = RID(GetWrenInterface()->GetNumFromMap("departure_planet", 0), EntityType::PLANET);
            task->current_planet = task->departure_planet;
            task->arrival_planet = RID(GetWrenInterface()->GetNumFromMap("arrival_planet", 0), EntityType::PLANET);
            task->payload_mass = GetWrenInterface()->GetNumFromMap("payload_mass", 0);
            task->pickup_expiration_time = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("departure_time_offset", 0));
            task->delivery_expiration_time = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("arrival_time_offset", 0));

            const char* on_success = GetWrenInterface()->GetStringFromMap("on_success", "INVALID");
            const char* on_fail = GetWrenInterface()->GetStringFromMap("on_fail", "INVALID");
            for (int i=0; i < 2; i++) delete[] next_options[i];
            next_options[0] = new char[strlen(on_success) + 1];
            next_options[1] = new char[strlen(on_fail) + 1];
            strcpy(next_options[0], on_success);
            strcpy(next_options[1], on_fail);
        }
        else if (strcmp(type, "wait") == 0) {
            await_type = WAIT;
            current.wait_until = GlobalGetNow() + timemath::Time(GetWrenInterface()->GetNumFromMap("wait_time", 0));

            const char* next = GetWrenInterface()->GetStringFromMap("next", "INVALID");
            delete[] next_options[0];  
            // deltete[] chrashes evcen though next_options[0] has been set intask ?!?
            next_options[0] = new char[strlen(next) + 1];
            strcpy(next_options[0], next);
        }
        else if (strcmp(type, "dialogue") == 0) {
            await_type = DAILOGUE;
            const char* speaker = GetWrenInterface()->GetStringFromMap("speaker", "NO SPEAKER");
            const char* text = GetWrenInterface()->GetStringFromMap("text", "NO TEXT");
            //const char* replies[] = { "<continue>" };

            dialogue_backlog.Append(GetQuestManager()->CreateDialogue(speaker, text, NULL, 0));
        }
        else if (strcmp(type, "dialogue choice") == 0) {
            await_type = DAILOGUE;
            const char* speaker = GetWrenInterface()->GetStringFromMap("speaker", "NO SPEAKER");
            const char* text = GetWrenInterface()->GetStringFromMap("text", "NO TEXT");

            GetWrenInterface()->PrepareMap("answer_choices");
            int replies_num = wrenGetListCount(vm, 2);
            const char** replies = new const char*[replies_num];
            for (int i=0; i < replies_num; i++) {
                wrenGetListElement(vm, 2, i, 1);
                replies[i] = wrenGetSlotString(vm, 1);
            }
            
            GetWrenInterface()->PrepareMap("answer_routes");
            if (wrenGetListCount(vm, 2) < replies_num) {
                replies_num = wrenGetListCount(vm, 2);
            }
            for (int i=0; i < replies_num; i++) {
                wrenGetListElement(vm, 2, i, 1);
                const char* option_route = wrenGetSlotString(vm, 1);
                next_options[i] = new char[strlen(option_route)];
                strcpy(next_options[i], option_route);
            }

            current.dialogue = GetQuestManager()->CreateDialogue(speaker, text, replies, replies_num);
            delete[] replies;
        }
        else if (strcmp(type, "goto") == 0) {
            const char* next = GetWrenInterface()->GetStringFromMap("next", "INVALID");
            wrenEnsureSlots(vm, 2);
            wrenSetSlotHandle(vm, 0, quest_instance_handle);
            wrenSetSlotString(vm, 1, next);
            if(!GetWrenInterface()->CallFunc(state_set_handle)) return;
            
            // call next_handle
            wrenEnsureSlots(vm, 1);
            wrenSetSlotHandle(vm, 0, quest_instance_handle);
            if(!GetWrenInterface()->CallFunc(next_handle)) return;

            _NextTask();
        }
        else if (strcmp(type, "gain money") == 0) {
            int faction = GetWrenInterface()->GetNumFromMap("faction", 0);
            cost_t delta = GetWrenInterface()->GetNumFromMap("ammount", 0);
            GetFactions()->CompleteTransaction(faction, delta);
        }
        else if (strcmp(type, "gain module") == 0) {
            const char* module_string_id = GetWrenInterface()->GetStringFromMap("module", "");
            int planet_index = GetWrenInterface()->GetNumFromMap("location", 0);
            RID module_id = GetGlobalState()->GetFromStringIdentifier(module_string_id);
            ShipModuleSlot free_slot = GetPlanetByIndex(planet_index)->GetFreeModuleSlot();
            if (free_slot.IsValid()) {
                free_slot.SetSlot(module_id);
            }
        }
        else if (strcmp(type, "gain reputation") == 0) {
            GetWrenInterface()->GetStringFromMap("faction", "");
            GetWrenInterface()->GetNumFromMap("ammount", 0);
            NOT_IMPLEMENTED
        }
        
        /*if (await_type == DONE) {
            ERROR("Implicit done reached => no condition met");
        }*/

        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, quest_instance_handle);
        if(!GetWrenInterface()->CallFunc(next_result_handle)) return;
        next_result_type = wrenGetSlotType(vm, 0);
    }
}
