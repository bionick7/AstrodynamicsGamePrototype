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
    quest->coroutine_instance_handle = NULL;
    quest->coroutine_call_handle = NULL;

    quest->id = GetInvalidId();

    quest->await_type = Quest::NOT_STARTED;
    quest->step = 0;
}

Quest::Quest() {
    ClearQuest(this);
}

void Quest::AttachTemplate(const WrenQuestTemplate* p_wren_interface) {
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

        task->departure_planet = RID(GetWrenInterface()->GetNumFromMap("departure_planet", 0), EntityType::PLANET);
        task->current_planet = task->departure_planet;
        task->arrival_planet = RID(GetWrenInterface()->GetNumFromMap("arrival_planet", 0), EntityType::PLANET);
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

bool Quest::IsValid() const {
    return wren_interface != NULL;
    /*return 
        wren_interface != NULL
        && quest_instance_handle != NULL
        && coroutine_instance_handle != NULL
        && coroutine_call_handle != NULL
        && IsIdValid(id)
    ;*/
}
