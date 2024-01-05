#include "wren_interface.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "string_builder.hpp"
#include "ship.hpp"

bool _WrenCallEmptyMethod(WrenHandle* class_handle, const char* signature) {
	WrenVM* vm = GetWrenVM();
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, class_handle);
	WrenHandle* id_caller = wrenMakeCallHandle(vm, signature);
	WrenInterpretResult call_result = wrenCall(vm, id_caller);
	wrenReleaseHandle(vm, id_caller);
	return call_result == WREN_RESULT_SUCCESS;
}

void WrenQuestTemplate::Attach(WrenHandle* p_class_handle) {
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

WrenQuestTemplate::~WrenQuestTemplate() {
	WrenVM* vm = GetWrenVM();
	wrenReleaseHandle(vm, class_handle);
	delete[] id;
}

WrenInterface::WrenInterface() {

}

WrenInterface::~WrenInterface() {

}

bool _AssertSlotType(WrenVM* vm, int slot, WrenType type) {
	if (wrenGetSlotCount(vm) <= slot) {
		wrenEnsureSlots(vm, 1);
		wrenSetSlotString(vm, 0, "Not enough slots provided");
		wrenAbortFiber(vm, 0);
		return false;
	}
	if (wrenGetSlotType(vm, slot) != type) {
		wrenEnsureSlots(vm, 1);
		char error_msg[40];
		sprintf(error_msg, "Wrong argument type at slot %d", slot);
		wrenSetSlotString(vm, 0, error_msg);
		wrenAbortFiber(vm, 0);
		return false;
	}
	return true;
}

void QuestInterfaceIsTaskPossible(WrenVM* vm) {
	NOT_IMPLEMENTED  // Details are blurry, bring more control to the wren side
	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, true);
}

static void WriteFn(WrenVM* vm, const char* text) {
	if (strcmp(text, "\n") != 0) {
        LogImpl(LOG_INVALID_FILE, LOG_INVALID_LINE, LOGTYPE_WRENINFO, text);
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
    LogImpl(LOG_INVALID_FILE, LOG_INVALID_LINE, LOGTYPE_WRENERROR, "Tried to call non implented foreign function");
}

void DefaultConstructor(WrenVM* vm) {

}

void DefaultDestructor(void* data) {

}

void DefaultMethod(WrenVM* vm) {
	wrenEnsureSlots(vm, 1);
	wrenSetSlotNull(vm, 0);
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
		return &DefaultMethod;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(module, "meta") == 0) {
		return &DefaultMethod;
	}
#endif // WREN_OPT_META

	// Only gets called on startup
	/*if (strcmp(className, "Quest") == 0) {
		if (strcmp(signature, "is_task_possible(_)") == 0) {
			return &QuestInterfaceIsTaskPossible;
		} else if (strcmp(signature, "gain_money(_)") == 0) {
			return &QuestInterfaceGainMoney;
		} else if (strcmp(signature, "gain_item(_,_)") == 0) {
			return &QuestInterfaceGainItem;
		} else if (strcmp(signature, "gain_reputation(_,_)") == 0) {
			return &QuestInterfaceGainReputation;
		} else if (strcmp(signature, "gain_ship_impl(_,_)") == 0) {
			return &QuestInterfaceGainShip;
		}
		ERROR("Unsupported foreign method '%s.%s'", className, signature)
	}*/
	ERROR("Unsupported foreign class '%s'", className)
	return &DefaultMethod;
}

WrenForeignClassMethods BindForeignClass(
	WrenVM* vm,
	const char* module,
	const char* className) 
{
	WrenForeignClassMethods res = { 0 };
	res.allocate = NULL;
	res.finalize = NULL;

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
	quests = new WrenQuestTemplate[fps.count];

	wrenInterpret(vm, "_internals", 
		"class Internals {\n"
		"\n"
		"}\n"
	);
	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, "_internals", "Internals", 0);
	internals_class_handle = wrenGetSlotHandle(vm, 0);

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
		if (wrenGetSlotType(vm, 0) != WREN_TYPE_STRING) {
			ERROR("'class_name' in '%s' (%s) must be a string identical to the class name", module_name, fps.paths[i])
			continue;
		}
		const char* class_name = wrenGetSlotString(vm, 0);
		wrenGetVariable(vm, module_name, class_name, 0);

		if (!wrenHasVariable(vm, module_name, class_name)) {
			ERROR("'class_name' in '%s' (%s) must be a string identical to the class name", module_name, fps.paths[i])
			continue;
		}
		WrenHandle* class_handle = wrenGetSlotHandle(vm, 0);
		quests[valid_quest_count++].Attach(class_handle);
	}
	UnloadDirectoryFiles(fps);
	return valid_quest_count;

	//INFO("start")
	//INFO("end")
}


WrenQuestTemplate* WrenInterface::GetWrenQuest(const char* query_id) const {
	// when surpassing ~50 of these, switch to a map or so.
	for(int i=0; i < valid_quest_count; i++) {
		if (strcmp(quests[i].id, query_id) == 0) {
			return &quests[i];
		}
	}
	return NULL;
}

WrenQuestTemplate* WrenInterface::GetRandomWrenQuest() const {
	if (valid_quest_count <= 0) {
		return NULL;
	}
	return &quests[GetRandomValue(0, valid_quest_count - 1)];
}

bool WrenInterface::CallFunc(WrenHandle* func_handle) const {
	WrenInterpretResult call_result = wrenCall(vm, func_handle);
	if (call_result == WREN_RESULT_COMPILE_ERROR) {
		ERROR("compilation error in function call")
	}
	if (call_result == WREN_RESULT_RUNTIME_ERROR) {
		ERROR("runntime error in function call")
	}
	// Handle errors
	return call_result == WREN_RESULT_SUCCESS;
}

void WrenInterface::MoveSlot(int from, int to) const {
	WrenHandle* handle = wrenGetSlotHandle(vm, from);
	wrenSetSlotHandle(vm, to, handle);
	wrenReleaseHandle(vm, handle);
}

bool WrenInterface::PrepareMap(const char* key) const {
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

double WrenInterface::GetNumFromMap(const char* key, double def) const {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_NUM) return def;
    return wrenGetSlotDouble(vm, 2);
}

bool WrenInterface::GetBoolFromMap(const char* key, bool def) const {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_BOOL) return def;
    return wrenGetSlotBool(vm, 2);
}

const char* WrenInterface::GetStringFromMap(const char* key, const char* def) const {
    // assumes map at position 0 and nothing else set
    if (!PrepareMap(key)) return def;
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_STRING) return def;
    return wrenGetSlotString(vm, 2);
}

void WrenInterface::_MapAsDataNodePopulateList(DataNode *dn, const char* key) const {
	// Will leave slots intact
	// reuse key slot to store list value
	const int DICT_SLOT = 0;
	const int KEYLIST_SLOT = 1;
	const int ELEM_SLOT = 2;
	const int VALUE_SLOT = 3;
	
	WrenHandle* dict_handle = wrenGetSlotHandle(vm, DICT_SLOT);
	WrenHandle* key_list_handle = wrenGetSlotHandle(vm, KEYLIST_SLOT);

	int list_count = wrenGetListCount(vm, VALUE_SLOT);
	if (list_count == 0) {
		dn->SetArray(key, list_count);
		return;
	}
	wrenGetListElement(vm, VALUE_SLOT, 0, ELEM_SLOT);
	if (wrenGetSlotType(vm, ELEM_SLOT) == WREN_TYPE_MAP) {  // Will not support mixed arrays
		NOT_IMPLEMENTED  // Current method disallows recursion
		dn->SetArrayChild(key, list_count);
		WrenHandle* list_handle = wrenGetSlotHandle(vm, VALUE_SLOT);
		for (int i=0; i < list_count; i++) {
			wrenGetListElement(vm, VALUE_SLOT, i, ELEM_SLOT);
			MoveSlot(ELEM_SLOT, 0);
			DataNode* child = dn->SetArrayElemChild(key, i, DataNode());
			MapAsDataNode(child);

			// Restore state
			wrenEnsureSlots(vm, 4);
			wrenSetSlotHandle(vm, DICT_SLOT, dict_handle);
			wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
			wrenSetSlotHandle(vm, VALUE_SLOT, list_handle);
		}
		return;
	}
	dn->SetArray(key, list_count);
	for (int i=0; i < list_count; i++) {
		wrenGetListElement(vm, VALUE_SLOT, i, ELEM_SLOT);
		WrenType value_type = wrenGetSlotType(vm, ELEM_SLOT);
		switch (value_type) {
		case WREN_TYPE_BOOL: {
			bool value = wrenGetSlotBool(vm, ELEM_SLOT);
			dn->SetArrayElem(key, i, value ? "y" : "n");
			break;}
		case WREN_TYPE_NUM: {
			double value = wrenGetSlotDouble(vm, ELEM_SLOT);
			if (std::fmod(value, 1.0) == 0.0) {
				dn->SetArrayElemI(key, i, (int)value);
			} else {
				dn->SetArrayElemF(key, i, value);
			}
			break;}
		case WREN_TYPE_NULL: {
			dn->SetArrayElem(key, i, "NONE");
			break;}
		case WREN_TYPE_STRING: {
			const char* value = wrenGetSlotString(vm, ELEM_SLOT);
			dn->SetArrayElem(key, i, value);
			break;}
		default: break; // Unsupported types
		}
	}

	wrenEnsureSlots(vm, 4);
	wrenSetSlotHandle(vm, DICT_SLOT, dict_handle);
	wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
}

bool WrenInterface::MapAsDataNode(DataNode *dn) const {
    // assumes map at position 0
	// cannot be called from within a foreign function
	// Will erase slots
	const int DICT_SLOT = 0;
	const int KEYLIST_SLOT = 1;
	const int KEY_SLOT = 2;
	const int VALUE_SLOT = 3;

    if (wrenGetSlotCount(vm) < 1) {
        return false;
    }
    if (wrenGetSlotType(vm, 0) != WREN_TYPE_MAP) {
        return false;
    }
	WrenHandle* dict_handle = wrenGetSlotHandle(vm, 0);
	WrenHandle* keys_callhandle = wrenMakeCallHandle(vm, "keys");
	CallFunc(keys_callhandle);
	WrenHandle* toList_callhandle = wrenMakeCallHandle(vm, "toList");
	CallFunc(toList_callhandle);
	WrenHandle* key_list_handle = wrenGetSlotHandle(vm, 0);
	wrenReleaseHandle(vm, keys_callhandle);
	wrenReleaseHandle(vm, toList_callhandle);

    wrenEnsureSlots(vm, 4);
	wrenSetSlotHandle(vm, DICT_SLOT, dict_handle);
	wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
	int dict_count = wrenGetListCount(vm, KEYLIST_SLOT);
	for (int i=0; i < dict_count; i++) {
		wrenGetListElement(vm, KEYLIST_SLOT, i, KEY_SLOT);
		if (wrenGetSlotType(vm, KEY_SLOT) != WREN_TYPE_STRING) {
			ERROR("Expected string key, not '%d' in wren datanode", (int) wrenGetSlotType(vm, KEY_SLOT))
			continue;
		}
		const char* key = wrenGetSlotString(vm, KEY_SLOT);
		wrenGetMapValue(vm, DICT_SLOT, KEY_SLOT, VALUE_SLOT);
		WrenType value_type = wrenGetSlotType(vm, VALUE_SLOT);
		switch (value_type) {
		case WREN_TYPE_BOOL: {
			bool value = wrenGetSlotBool(vm, VALUE_SLOT);
			dn->Set(key, value ? "y" : "n");
			break;}
		case WREN_TYPE_NUM: {
			double value = wrenGetSlotDouble(vm, VALUE_SLOT);
			if (std::fmod(value, 1.0) == 0.0) {
				dn->SetI(key, (int)value);
			} else {
				dn->SetF(key, value);
			}
			break;}
		case WREN_TYPE_NULL: {
			dn->Set(key, "NONE");
			break;}
		case WREN_TYPE_STRING: {
			const char* value = wrenGetSlotString(vm, VALUE_SLOT);
			dn->Set(key, value);
			break;}
		case WREN_TYPE_LIST: {
			_MapAsDataNodePopulateList(dn, key);
			break;}
		case WREN_TYPE_MAP: {
			NOT_IMPLEMENTED  // Current method disallows recursion
			DataNode* child = dn->SetChild(key, DataNode());
			MoveSlot(VALUE_SLOT, 0);
			bool success = MapAsDataNode(child);
			wrenEnsureSlots(vm, 4);
			wrenSetSlotHandle(vm, DICT_SLOT, dict_handle);
			wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
			break;}
		default: {
			ERROR("Unexpected vale type in wren datanode: '%d'", (int) value_type)
			break;}
		}
	}
    return true;
}

bool WrenInterface::DataNodeToMap(const DataNode* dn) const {
	// Puts map into slot 0
	// map will only contain strings
	const int MAP_SLOT = 0;
	const int KEY_SLOT = 1;
	const int VALUE_SLOT = 2;
	const int ELEM_SLOT = 3;
	wrenEnsureSlots(vm, 4);
	wrenSetSlotNewMap(vm, MAP_SLOT);
	for(int i=0; i < dn->GetFieldCount(); i++) {
		const char* key = dn->GetKey(i);
		const char* value = dn->Get(key);
		wrenSetSlotString(vm, KEY_SLOT, key);
		wrenSetSlotString(vm, VALUE_SLOT, value);
		wrenSetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
	}
	for(int i=0; i < dn->GetChildCount(); i++) {
		const char* key = dn->GetChildKey(i);
		WrenHandle* handle_map = wrenGetSlotHandle(vm, MAP_SLOT);

		DataNodeToMap(dn->GetChild(key));
		MoveSlot(MAP_SLOT, VALUE_SLOT);

		// restore current map
		wrenSetSlotHandle(vm, MAP_SLOT, handle_map);
		wrenReleaseHandle(vm, handle_map);

		wrenSetSlotString(vm, KEY_SLOT, key);
		wrenSetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
	}
	for(int i=0; i < dn->GetArrayCount(); i++) {
		const char* key = dn->GetArrayKey(i);
		wrenSetSlotString(vm, KEY_SLOT, key);
		wrenSetSlotNewList(vm, VALUE_SLOT);
		for(int j=dn->GetArrayLen(key); j >= 0; j--) {  // Minimizes list resizing
			wrenSetSlotString(vm, ELEM_SLOT, dn->GetArray(key, j));
			wrenInsertInList(vm, VALUE_SLOT, j, ELEM_SLOT);
		}
		wrenSetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
	}
	for(int i=0; i < dn->GetChildArrayCount(); i++) {
		const char* key = dn->GetChildArrayKey(i);
		wrenSetSlotString(vm, KEY_SLOT, key);
		WrenHandle* handle_map = wrenGetSlotHandle(vm, MAP_SLOT);
		for(int j=dn->GetArrayLen(key); j >= 0; j--) {  // Minimizes list resizing

			DataNodeToMap(dn->GetArrayChild(key, j));
			wrenInsertInList(vm, VALUE_SLOT, j, MAP_SLOT);
		}
		// restore current map
		wrenSetSlotHandle(vm, MAP_SLOT, handle_map);
		wrenReleaseHandle(vm, handle_map);

		wrenSetSlotString(vm, KEY_SLOT, key);
		wrenSetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
	}
	return true;
}

WrenVM* GetWrenVM() {
    return GetWrenInterface()->vm;
}
