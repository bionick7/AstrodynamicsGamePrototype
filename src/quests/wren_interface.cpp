#include "wren_interface.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "core/string_builder.hpp"

bool _WrenCallEmptyMethod(WrenHandle* class_handle, const char* signature) {
	WrenVM* vm = GetWrenVM();
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, class_handle);
	WrenHandle* id_caller = wrenMakeCallHandle(vm, signature);
	WrenInterpretResult call_result = wrenCall(vm, id_caller);
	wrenReleaseHandle(vm, id_caller);
	return call_result == WREN_RESULT_SUCCESS;
}

void WrenQuest::Attach(WrenHandle* p_class_handle) {
	WrenVM* vm = GetWrenVM();
	class_handle = p_class_handle;

	_WrenCallEmptyMethod(class_handle, "id");
	const char* id_ = wrenGetSlotString(vm, 0);
	id = new char[strlen(id_) + 1];
	strcpy(id, id_);
	
	_WrenCallEmptyMethod(class_handle, "challenge_level");
	challenge_level = (int) floor(wrenGetSlotDouble(vm, 0));

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, class_handle);
	/*WrenHandle* test_handle = wrenMakeCallHandle(vm, "test()");
	wrenCall(vm, test_handle);
	wrenReleaseHandle(vm, test_handle);*/
}

WrenQuest::~WrenQuest() {
	WrenVM* vm = GetWrenVM();
	wrenReleaseHandle(vm, class_handle);
	delete[] id;
}

WrenInterface::WrenInterface() {

}

WrenInterface::~WrenInterface() {

}

void QuestInterfaceIsTaskPossible(WrenVM* vm) {
	NOT_IMPLEMENTED
	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, true);
}

void QuestInterfacePayMoney(WrenVM* vm) {
	cost_t delta = (cost_t) wrenGetSlotDouble(vm, 0);
	GlobalGetState()->CompleteTransaction(delta, "");
}

void QuestInterfacePayItem(WrenVM* vm) {
	NOT_IMPLEMENTED
}

void QuestInterfacePayReputation(WrenVM* vm) {
	NOT_IMPLEMENTED
}

static void WriteFn(WrenVM* vm, const char* text) {
	if (strcmp(text, "\n") != 0) {
        LogImpl("wren", -1, LOGTYPE_WRENINFO, text);
	}
}

void ErrorFn(WrenVM* vm, WrenErrorType errorType,
	const char* module, const int p_line,
	const char* msg)
{
	if (module == nullptr) {
		module = "---";
	}

	if (msg == nullptr) {
		msg = "---";
	}

	int line = p_line;

	switch (errorType)
	{
	case WREN_ERROR_COMPILE:
        LogImpl(module, p_line, LOGTYPE_WRENERROR, 
            "Wren compilation error: \"%s\"", msg
        );
		break;
	case WREN_ERROR_STACK_TRACE:
        LogImpl(module, p_line, LOGTYPE_WRENERROR, 
            "@ \"%s\" ^^^", msg
        );
	case WREN_ERROR_RUNTIME:
        LogImpl(module, p_line, LOGTYPE_WRENERROR, 
            "Wren runtime error  \"%s\"", msg
        );
		break;
	}
}

void NotImplementedFn(WrenVM* vm) {
    LogImpl(" - ", -1, LOGTYPE_WRENERROR, "Tried to call non implented foreign function");
}

void DefaultConstructor(WrenVM* vm) {

}

void DefaultDestructor(void* data) {

}

WrenForeignMethodFn BindForeignMethod(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature)
{
	// Skip errormessage in these cases
#ifdef WREN_OPT_RANDOM
	if (strcmp(module, "random") == 0) {
		return nullptr;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(module, "meta") == 0) {
		return nullptr;
	}
#endif // WREN_OPT_META

	// Only gets called on startup
	if (strcmp(className, "Quest") == 0) {
		if (strcmp(signature, "is_task_possible(_)") == 0) {
			return &QuestInterfaceIsTaskPossible;
		} else if (strcmp(signature, "pay_money(_)") == 0) {
			return &QuestInterfacePayMoney;
		} else if (strcmp(signature, "pay_item(_)") == 0) {
			return &QuestInterfacePayItem;
		} else if (strcmp(signature, "pay_reputation(_)") == 0) {
			return &QuestInterfacePayReputation;
		}
		ERROR("Unsupported foreign method '%s.%s'", className, signature)
	}
	ERROR("Unsupported foreign class '%s'", className)
	return nullptr;
}

WrenForeignClassMethods BindForeignClass(
	WrenVM* vm,
	const char* module,
	const char* className) 
{
	WrenForeignClassMethods res = { 0 };
	res.allocate = nullptr;
	res.finalize = nullptr;

	// Skip errormessage in these cases
#ifdef WREN_OPT_RANDOM
	if (strcmp(module, "random") == 0) {
		return res;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(module, "meta") == 0) {
		return res;
	}
#endif // WREN_OPT_META

    return res;
}

static void LoadModuleComplete(WrenVM* vm, const char* module, WrenLoadModuleResult result) {
    free((void*) result.source);
}

WrenLoadModuleResult LoadModule(WrenVM* vm, const char* path) {
	WrenLoadModuleResult res = { 0 };

	res.source = "";
	// Skip errormessage in these cases
#ifdef WREN_OPT_RANDOM
	if (strcmp(name, "random") == 0) {
		return res;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(name, "meta") == 0) {
		return res;
	}
#endif // WREN_OPT_META

	res.onComplete = LoadModuleComplete;

    StringBuilder sb = StringBuilder();
    sb.Add("resources/wren/");
    sb.Add(path);
    sb.Add(".wren");
	if (FileExists(sb.c_str)) {
        res.source = LoadFileText(sb.c_str);
	} else {
		ERROR("No such module %s (looking for file '%s')", path, sb.c_str)
	}
	return res;
}

void WrenInterface::MakeVM() {
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.writeFn = &WriteFn;
	config.errorFn = &ErrorFn;
	config.bindForeignMethodFn = &BindForeignMethod;
	config.bindForeignClassFn = &BindForeignClass;
	config.loadModuleFn = &LoadModule;
	vm = wrenNewVM(&config);
}

int WrenInterface::LoadWrenQuests() {
	FilePathList fps = LoadDirectoryFilesEx("resources/wren/quests", ".wren", true);
	delete[] quests;
	quests = new WrenQuest[fps.count];
	for (int i=0; i < fps.count; i++) {
		const char* module_name = GetFileNameWithoutExt(fps.paths[i]);
		INFO("Loading %s", module_name);
		wrenInterpret(vm, module_name, LoadFileText(fps.paths[i]));

		wrenEnsureSlots(vm, 1);
		//INFO("wren has '%s' in module '%s' ? %s", "ExampleQuest", name, wrenHasVariable(vm, name, "ExampleQuest") ? "yes" : "no");
		if (!wrenHasVariable(vm, module_name, "class_name")) {
			ERROR("quest in '%s' (%s) has no 'class_name' variable", module_name, fps.paths[i])
			continue;
		}
		wrenGetVariable(vm, module_name, "class_name", 0);
		const char* class_name = wrenGetSlotString(vm, 0);
		wrenGetVariable(vm, module_name, class_name, 0);
		WrenHandle* class_handle = wrenGetSlotHandle(vm, 0);
		quests[valid_quest_count++].Attach(class_handle);
	}
	UnloadDirectoryFiles(fps);
	return valid_quest_count;

	//INFO("start")
	//INFO("end")
}


WrenQuest* WrenInterface::GetWrenQuest(const char* query_id) {
	// when surpassing ~50 of these, switch to a map or so.
	for(int i=0; i < valid_quest_count; i++) {
		if (strcmp(quests[i].id, query_id) == 0) {
			return &quests[i];
		}
	}
	return NULL;
}

WrenQuest* WrenInterface::GetRandomWrenQuest() {
	if (valid_quest_count <= 0) {
		return NULL;
	}
	return &quests[GetRandomValue(0, valid_quest_count - 1)];
}

bool WrenInterface::CallFunc(WrenHandle* func_handle) {
	WrenInterpretResult call_result = wrenCall(vm, func_handle);
	// Handle errors
	return call_result == WREN_RESULT_SUCCESS;
}

bool WrenInterface::PrepareMap(const char* key) {
    // assumes map at position 0 and nothing else set
    // puts result in slot 2
    if (wrenGetSlotCount(vm) == 0) {
        return false;
    }
    if (wrenGetSlotType(vm, 0) != WREN_TYPE_MAP) {
        return false;
    }
    wrenEnsureSlots(vm, 3);
    wrenSetSlotString(vm, 1, key);
    if (!wrenGetMapContainsKey(vm, 0, 1)) {
        return false;
    }
    wrenGetMapValue(vm, 0, 1, 2);
    return true;
}

double WrenInterface::GetNumFromMap(const char* key, double def) {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_NUM) return def;
    return wrenGetSlotDouble(vm, 2);
}

bool WrenInterface::GetBoolFromMap(const char* key, bool def) {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_BOOL) return def;
    return wrenGetSlotBool(vm, 2);
}

const char* WrenInterface::GetStringFromMap(const char* key, const char* def) {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_STRING) return def;
    return wrenGetSlotString(vm, 2);
}


WrenVM* GetWrenVM() {
    return GetWrenInterface()->vm;
}

// resources wren foreign quest_base.wren
// resources wren foreign quest_base.wren