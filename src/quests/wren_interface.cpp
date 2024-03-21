#include "wren_interface.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "string_builder.hpp"
#include "ship.hpp"
#include "assets.hpp"

static const char* WREN_TYPENAMES[] = {
	"Bool",  // WREN_TYPE_BOOL
	"Num",  // WREN_TYPE_NUM
	"Foreign",  // WREN_TYPE_FOREIGN
	"List",  // WREN_TYPE_LIST
	"Map",  // WREN_TYPE_MAP
	"Null",  // WREN_TYPE_NULL
	"String",  // WREN_TYPE_STRING
	"Unknown", // WREN_TYPE_UNKNOWN
};

bool _AssertSlot(WrenVM* vm, int slot, WrenType type) {
	if (wrenGetSlotCount(vm) <= slot) {
		wrenEnsureSlots(vm, 1);
		char error_msg[40];
		sprintf(error_msg, "Expected argument in slot %d", slot);
		wrenSetSlotString(vm, 0, error_msg);
		wrenAbortFiber(vm, 0);
		return false;
	}
	if (wrenGetSlotType(vm, slot) != type) {
		wrenEnsureSlots(vm, 1);
		char error_msg[71];  // for up slot 999
		INFO("%d", wrenGetSlotType(vm, slot))
		sprintf(error_msg, 
			"Expected argument of type '%s' in slot %d. Instead got '%s'", 
			WREN_TYPENAMES[type], slot, WREN_TYPENAMES[wrenGetSlotType(vm, slot)]
		);
		wrenSetSlotString(vm, 0, error_msg);
		wrenAbortFiber(vm, 0);
		return false;
	}
	return true;
}

namespace function_call_globals {
	int arg_count;
	WrenType arg_types[16];
	WrenType ret_type;

	union Argument {
		double as_num;
		bool as_bool;
		const char* as_string;

		operator double() const { return as_num; }
		operator int() const { return as_num; }
		operator bool() const { return as_bool; }
		operator const char*() const { return as_string; }
	};

	Argument (*func)(Argument a[]);
}

void _WrenFunctionName(WrenVM* vm) {
	function_call_globals::Argument args[16];
	for (int i=0; i < function_call_globals::arg_count; i++) {
		_AssertSlot(vm, i+1, function_call_globals::arg_types[i]);
	}
	for (int i=0; i < function_call_globals::arg_count; i++) {
		switch(function_call_globals::arg_types[i]) {
		case WREN_TYPE_NUM: 
			args[i].as_num = wrenGetSlotDouble(vm, i+1);
			break;
		case WREN_TYPE_BOOL: 
			args[i].as_bool = wrenGetSlotBool(vm, i+1);
			break;
		case WREN_TYPE_STRING: 
			args[i].as_string = wrenGetSlotString(vm, i+1);
			break;
		default:
			NOT_IMPLEMENTED
		}
	}

	function_call_globals::Argument res = function_call_globals::func(args);
	
	switch(function_call_globals::ret_type) {
	case WREN_TYPE_NUM: 
		wrenSetSlotDouble(vm, 0, res.as_bool);
		break;
	case WREN_TYPE_BOOL: 
		wrenSetSlotBool(vm, 0, res.as_bool);
		break;
	case WREN_TYPE_STRING: 
		wrenSetSlotString(vm, 0, res.as_string);
		break;
	case WREN_TYPE_NULL:
		break;
	default:
		NOT_IMPLEMENTED
	}
}

bool _IsSignature(const char* method_name, int arg_count, const char* test_signature) {
	ASSERT(strlen(method_name) < 100)
	ASSERT(arg_count <= 16)
	ASSERT(arg_count >= 0)
	char method_signature[134];
	if (arg_count == 0) {
		sprintf(method_signature, "%s()", method_name);
		return strcmp(method_signature, test_signature) == 0 || strcmp(method_name, test_signature) == 0;
	} else {
		char comma_array[] = "_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,";
		comma_array[2*arg_count-1] = '\0';
		sprintf(method_signature, "%s(%s)", method_name, comma_array);
	}
	//INFO("'%s' =? '%s' : %s", method_signature, test_signature, match ? "y": "n")
	return strcmp(method_signature, test_signature) == 0;
}

#define BIND(class_name, method_name, operation, T_ret, ...) {\
	int arg_count = sizeof((WrenType[]) {WREN_TYPE_NULL, __VA_ARGS__}) / sizeof(WrenType) - 1; \
	if (strcmp(className, #class_name) == 0 && _IsSignature(#method_name, arg_count, signature)) { \
		return &function_call_globals::_wren_outer_##class_name##method_name;\
	} \
}

#define _DEF_FUNC_INTERNAL(class_name, method_name, operation) \
	Argument _wren_internal_##class_name##method_name(Argument args[]) { \
		Argument res; \
		operation \
		return res; \
	} \

#define DEF_FUNC(class_name, method_name, operation, T_ret, ...) \
	_DEF_FUNC_INTERNAL(class_name, method_name, operation) \
	void _wren_outer_##class_name##method_name(WrenVM* vm) { \
		const static WrenType temp_arg_types[] = {WREN_TYPE_NULL, __VA_ARGS__}; \
		arg_count = sizeof(temp_arg_types) / sizeof(WrenType) - 1;\
		for(int i=0; i < arg_count; i++) { arg_types[i] = temp_arg_types[i+1]; }\
		ret_type = T_ret;\
		func = &_wren_internal_##class_name##method_name; \
		_WrenFunctionName(vm); \
	} \

#define X_WREN_FUNCTION_BINDS \
	X(Game, now, { res.as_num = GlobalGetNow().Seconds(); }, WREN_TYPE_NUM) \
	X(Game, spawn_quest, { GetWrenInterface()->PushQuest(args[0].as_string); }, WREN_TYPE_NULL, WREN_TYPE_STRING) \


namespace function_call_globals {
	#define X(...) DEF_FUNC(__VA_ARGS__)
	X_WREN_FUNCTION_BINDS
	#undef X
}

void _AllocateShip(WrenVM* vm) {
	if(!_AssertSlot(vm, 0, WREN_TYPE_NUM)) return;
	int rid = wrenGetSlotDouble(vm, 0);
	if (!wrenHasModule(vm, "foreign/ship") || !wrenHasVariable(vm, "foreign/ship", "Ship")) {
		wrenEnsureSlots(vm, 1);
		wrenSetSlotString(vm, 0, "Ship class not known");
		wrenAbortFiber(vm, 0);
		return;
	}
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "foreign/ship", "Ship", 1);
	Ship* ship = GetShip(RID(rid));
	void* ptr = wrenSetSlotNewForeign(vm, 0, 1, sizeof(RID));
	*((RID*)ptr) = RID(rid);
}

void _FinalizeShip(void* data) { 
	RID rid = *(RID*)data;
	if(!IsIdValidTyped(rid, EntityType::SHIP)) {
		return;
	}
	Ship* ship = GetShip(rid);
	//ship->wren_handle = NULL;
}

void GameNow(WrenVM* vm) {
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, GlobalGetNow().Seconds());
}

void GameHohmannTF(WrenVM* vm) {
	// 0 is reserved for instance
	if(!_AssertSlot(vm, 1, WREN_TYPE_NUM)) return;
	if(!_AssertSlot(vm, 2, WREN_TYPE_NUM)) return;
	if(!_AssertSlot(vm, 3, WREN_TYPE_NUM)) return;
	const Orbit* from = &GetPlanetByIndex(wrenGetSlotDouble(vm, 1))->orbit;
	const Orbit* to = &GetPlanetByIndex(wrenGetSlotDouble(vm, 2))->orbit;
	timemath::Time t0 = timemath::Time(wrenGetSlotDouble(vm, 3));
	timemath::Time departure_t, arrival_t;
	double dv1, dv2;
	HohmannTransfer(
		from, to, t0,
		&departure_t, &arrival_t, &dv1, &dv2
	);

	wrenEnsureSlots(vm, 3);  // Good practice
	wrenSetSlotNewMap(vm, 0);
	wrenSetSlotString(vm, 1, "t1");
	wrenSetSlotDouble(vm, 2, departure_t.Seconds());
	wrenSetMapValue(vm, 0, 1, 2);
	wrenSetSlotString(vm, 1, "t2");
	wrenSetSlotDouble(vm, 2, arrival_t.Seconds());
	wrenSetMapValue(vm, 0, 1, 2);
	wrenSetSlotString(vm, 1, "dv1");
	wrenSetSlotDouble(vm, 2, dv1);
	wrenSetMapValue(vm, 0, 1, 2);
	wrenSetSlotString(vm, 1, "dv2");
	wrenSetSlotDouble(vm, 2, dv2);
	wrenSetMapValue(vm, 0, 1, 2);
}

void ShipSpawn(WrenVM* vm) {
	if(!_AssertSlot(vm, 1, WREN_TYPE_MAP)) return;
	GetWrenInterface()->MoveSlot(1, 0);
	DataNode dn;
	GetWrenInterface()->MapAsDataNode(&dn);
	RID id = GetShips()->AddShip(&dn);
	wrenSetSlotDouble(vm, 0, id.AsInt());
	_AllocateShip(vm);
}

void ShipKill(WrenVM* vm) {
	if(!_AssertSlot(vm, 1, WREN_TYPE_BOOL)) return;
	bool call_callback = wrenGetSlotBool(vm, 1);
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	if (!IsIdValid(ship)) return;
	GetShips()->KillShip(ship, call_callback);
}

void ShipSetStat(WrenVM* vm) {
	if(!_AssertSlot(vm, 1, WREN_TYPE_NUM)) return;
	if(!_AssertSlot(vm, 2, WREN_TYPE_NUM)) return;
	int stat_index = wrenGetSlotDouble(vm, 1);
	int stat_value = wrenGetSlotDouble(vm, 2);
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	if (!IsIdValid(ship)) return;
	if (stat_index < 0 || stat_index >= ship_stats::MAX) {
		wrenEnsureSlots(vm, 1);
		wrenSetSlotString(vm, 0, "invalid stat index");
		wrenAbortFiber(vm, 0);
		return;
	}
	GetShip(ship)->stats[stat_index] = stat_value;
}

void ShipGetStat(WrenVM* vm) {
	if(!_AssertSlot(vm, 1, WREN_TYPE_NUM)) return;
	int stat_index = wrenGetSlotDouble(vm, 1);
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	if (!IsIdValid(ship)) return;
	wrenEnsureSlots(vm, 1);
	if (stat_index < 0 || stat_index >= ship_stats::MAX) {
		wrenSetSlotString(vm, 0, "invalid stat index");
		wrenAbortFiber(vm, 0);
		return;
	}
	wrenSetSlotDouble(vm, 0, GetShip(ship)->stats[stat_index]);
}

void ShipGetPlans(WrenVM* vm) {
	RID ship_id = GetWrenInterface()->GetShipFormWrenObject();
	wrenEnsureSlots(vm, 1);
	if (!IsIdValid(ship_id)) {
		wrenSetSlotString(vm, 0, "Invalid ship");
		wrenAbortFiber(vm, 0);
		return;
	}
	Ship* ship = GetShip(ship_id);
	wrenSetSlotNewList(vm, 0);
	WrenHandle* list_handle = wrenGetSlotHandle(vm, 0);
	for(int i=0; i < ship->prepared_plans_count; i++) {
		DataNode dn;
		ship->prepared_plans[i].Serialize(&dn);
		GetWrenInterface()->DataNodeToMap(&dn);
		wrenSetSlotHandle(vm, 1, list_handle);
		wrenInsertInList(vm, 1, i, 0);
	}
	wrenSetSlotHandle(vm, 0, list_handle);
}

void ShipGetId(WrenVM* vm) {
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	wrenEnsureSlots(vm, 1);
	if (!IsIdValid(ship)) {
		wrenSetSlotString(vm, 0, "Invalid ship");
		wrenAbortFiber(vm, 0);
		return;
	}
	wrenSetSlotDouble(vm, 0, ship.AsInt());
}

void ShipGetName(WrenVM* vm) {
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	wrenEnsureSlots(vm, 1);
	if (!IsIdValid(ship)) {
		wrenSetSlotString(vm, 0, "Invalid ship");
		wrenAbortFiber(vm, 0);
		return;
	}
	wrenSetSlotString(vm, 0, GetShip(ship)->name);
}

void ShipExists(WrenVM* vm) {
	RID ship = GetWrenInterface()->GetShipFormWrenObject();
	wrenEnsureSlots(vm, 1);
	wrenSetSlotBool(vm, 0, IsIdValid(ship));
}

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
	/*// Cry about it
	for (WrenHandle** handle = &common_handles.ship_id; handle != &common_handles.END_HANDLE; handle++) {
		wrenReleaseHandle(vm, *handle);
	}
	wrenFreeVM(vm);*/
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
		return NULL;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(module, "meta") == 0) {
		return NULL;
	}
#endif // WREN_OPT_META

	// Only gets called on startup
	if (strcmp(className, "Game") == 0) {
		if (!isStatic) {
			ERROR("'Game' methods must be static")
		} else {
			if (strcmp(signature, "now") == 0) {
				return &GameNow;
			} else if (strcmp(signature, "hohmann_tf(_,_,_)") == 0) {
				return &GameHohmannTF;
			}
		}
		//ERROR("Unsupported foreign method '%s.%s'", className, signature)
	}
	
	if (strcmp(className, "Ship") == 0) {
		if (isStatic) {
			if (strcmp(signature, "spawn(_)") == 0) {
				return &ShipSpawn;
			}
		} else {
			if (strcmp(signature, "kill(_)") == 0) {
				return &ShipKill;
			} else if (strcmp(signature, "set_stat(_,_)") == 0) {
				return &ShipSetStat;
			} else if (strcmp(signature, "get_stat(_)") == 0) {
				return &ShipGetStat;
			} else if (strcmp(signature, "get_plans()") == 0) {
				return &ShipGetPlans;
			} else if (strcmp(signature, "id") == 0) {
				return &ShipGetId;
			} else if (strcmp(signature, "name") == 0) {
				return &ShipGetName;
			} else if (strcmp(signature, "exists") == 0) {
				return &ShipExists;
			}
		}
		//ERROR("Unsupported foreign method '%s.%s'", className, signature)
	}
	
	#define X(...) BIND(__VA_ARGS__)
	X_WREN_FUNCTION_BINDS
	#undef X

	ERROR("Unsupported foreign method '%s' in class '%s'", signature, className)
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

	if (strcmp(className, "Ship") == 0) {
		res.allocate = &_AllocateShip;
		res.finalize = &_FinalizeShip;
		return res;
	}
	ERROR("Unsupported foreign class '%s' in module '%s", className, module)

    return res;
}

static void LoadModuleComplete(WrenVM* vm, const char* module, WrenLoadModuleResult result) {
    free((void*) result.source);
}

WrenLoadModuleResult LoadModule(WrenVM* vm, const char* path) {
	WrenLoadModuleResult res = { 0 };

	res.source = NULL;
	// Skip errormessage in these cases
#ifdef WREN_OPT_RANDOM
	if (strcmp(path, "random") == 0) {
		return res;
	}
#endif // WREN_OPT_RANDOM
#ifdef WREN_OPT_META
	if (strcmp(path, "meta") == 0) {
		return res;
	}
#endif // WREN_OPT_META

	res.onComplete = LoadModuleComplete;

    StringBuilder sb = StringBuilder();
    sb.Add("resources/wren/");
    sb.Add(path);
    sb.Add(".wren");
	if (assets::HasTextResource(sb.c_str)) {
        res.source = assets::GetResourceText(sb.c_str);
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
	
	// Allocate Handles for Common Handles 
	common_handles.quest_notify = wrenMakeCallHandle(vm, "notify_event(_,_)");
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
		WrenInterpretResult res = wrenInterpret(vm, module_name, LoadFileText(fps.paths[i]));
		if (res != WREN_RESULT_SUCCESS) {
			ERROR("Could not load module '%s'", module_name)
			continue;
		}

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

	wrenGetVariable(vm, "_internals", "Internals", 0);
	// todo

	// Testing
	const char* tests_path = "resources/wren/testing.wren";
	if (assets::HasTextResource(tests_path)) {
		WrenInterpretResult res = wrenInterpret(vm, "testing", assets::GetResourceText(tests_path));
		if (wrenHasModule(vm, "testing") && wrenHasVariable(vm, "testing", "Tests")) {
			wrenEnsureSlots(vm, 1);
			wrenGetVariable(vm, "testing", "Tests", 0);
			common_handles.test_class = wrenGetSlotHandle(vm, 0);
		} else {
			common_handles.test_class = NULL;
		}
		common_handles.test_call_handle = wrenMakeCallHandle(vm, "tests()");
	} else {
		INFO("No file found at '%s'. Skipping testing", tests_path)
		common_handles.test_class = NULL;
	}

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

RID WrenInterface::GetShipFormWrenObject() const {
	// Kills the stack
	if(!_AssertSlot(vm, 0, WREN_TYPE_FOREIGN)) return GetInvalidId();
	return *((RID*) wrenGetSlotForeign(vm, 0));
}

void WrenInterface::NotifyShipEvent(RID ship, const char *event) {
	IDAllocatorList<Quest, EntityType::ACTIVE_QUEST>* active_quests = &GetQuestManager()->active_quests;
	for (auto iter = active_quests->GetIter(); iter; iter++) {
		wrenEnsureSlots(vm, 4);
		wrenSetSlotDouble(vm, 0, ship.AsInt());
		_AllocateShip(vm);

		wrenSetSlotNewList(vm, 2);
		wrenInsertInList(vm, 2, 0, 0);
		
		wrenSetSlotString(vm, 1, event);

		wrenSetSlotHandle(vm, 0, active_quests->Get(iter.GetId())->quest_instance_handle);
		wrenCall(vm, common_handles.quest_notify);
	}
}

void WrenInterface::PushQuest(const char *quest_id) {
	quest_queue.push(GetWrenQuest(quest_id));
}

void WrenInterface::Update() {
	while (!quest_queue.empty()) {
		GetQuestManager()->ForceQuest(quest_queue.front());
		quest_queue.pop();
	}
	
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
		dn->CreateArray(key, list_count);
		return;
	}
	wrenGetListElement(vm, VALUE_SLOT, 0, ELEM_SLOT);
	if (wrenGetSlotType(vm, ELEM_SLOT) == WREN_TYPE_MAP) {  // Will not support mixed arrays
		dn->CreatChildArray(key, list_count);
		WrenHandle* list_handle = wrenGetSlotHandle(vm, VALUE_SLOT);
		for (int i=0; i < list_count; i++) {
			wrenGetListElement(vm, VALUE_SLOT, i, ELEM_SLOT);
			MoveSlot(ELEM_SLOT, 0);
			DataNode* child = dn->InsertIntoChildArray(key, i);
			MapAsDataNode(child);

			// Restore state
			wrenEnsureSlots(vm, 4);
			wrenSetSlotHandle(vm, DICT_SLOT, dict_handle);
			wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
			wrenSetSlotHandle(vm, VALUE_SLOT, list_handle);
		}
		return;
	}
	dn->CreateArray(key, list_count);
	for (int i=0; i < list_count; i++) {
		wrenGetListElement(vm, VALUE_SLOT, i, ELEM_SLOT);
		WrenType value_type = wrenGetSlotType(vm, ELEM_SLOT);
		switch (value_type) {
		case WREN_TYPE_BOOL: {
			bool value = wrenGetSlotBool(vm, ELEM_SLOT);
			dn->InsertIntoArray(key, i, value ? "y" : "n");
			break;}
		case WREN_TYPE_NUM: {
			double value = wrenGetSlotDouble(vm, ELEM_SLOT);
			if (std::fmod(value, 1.0) == 0.0) {
				dn->InsertIntoArrayI(key, i, (int)value);
			} else {
				dn->InsertIntoArrayF(key, i, value);
			}
			break;}
		case WREN_TYPE_NULL: {
			dn->InsertIntoArray(key, i, "NONE");
			break;}
		case WREN_TYPE_STRING: {
			const char* value = wrenGetSlotString(vm, ELEM_SLOT);
			dn->InsertIntoArray(key, i, value);
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
	const int MAP_SLOT = 0;
	//const int KEYLIST_SLOT = 1;
	const int KEY_SLOT = 2;
	const int VALUE_SLOT = 3;

    if (wrenGetSlotCount(vm) < 1) {
        return false;
    }
    if (wrenGetSlotType(vm, 0) != WREN_TYPE_MAP) {
        return false;
    }
	WrenHandle* dict_handle = wrenGetSlotHandle(vm, 0);
	/*WrenHandle* keys_callhandle = wrenMakeCallHandle(vm, "keys");
	CallFunc(keys_callhandle);
	WrenHandle* toList_callhandle = wrenMakeCallHandle(vm, "toList");
	CallFunc(toList_callhandle);
	WrenHandle* key_list_handle = wrenGetSlotHandle(vm, 0);
	wrenReleaseHandle(vm, keys_callhandle);
	wrenReleaseHandle(vm, toList_callhandle);*/

    wrenEnsureSlots(vm, 4);
	wrenSetSlotHandle(vm, MAP_SLOT, dict_handle);
	//wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
	int dict_count = wrenGetMapCount(vm, MAP_SLOT);
	int iter=0;
	for ( ;; ) {
		//wrenGetListElement(vm, KEYLIST_SLOT, i, KEY_SLOT);
		wrenGetMapKey(vm, MAP_SLOT, &iter, KEY_SLOT);
		if (iter < 0) {
			return true;
		}
		if (wrenGetSlotType(vm, KEY_SLOT) != WREN_TYPE_STRING) {
			ERROR("Expected string key, not '%d' in wren datanode", (int) wrenGetSlotType(vm, KEY_SLOT))
			continue;
		}
		const char* key = wrenGetSlotString(vm, KEY_SLOT);
		wrenGetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
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
			DataNode* child = dn->SetChild(key);
			MoveSlot(VALUE_SLOT, 0);
			bool success = MapAsDataNode(child);
			wrenEnsureSlots(vm, 4);
			wrenSetSlotHandle(vm, MAP_SLOT, dict_handle);
			//wrenSetSlotHandle(vm, KEYLIST_SLOT, key_list_handle);
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
			wrenSetSlotString(vm, ELEM_SLOT, dn->GetArrayElem(key, j));
			wrenInsertInList(vm, VALUE_SLOT, j, ELEM_SLOT);
		}
		wrenSetMapValue(vm, MAP_SLOT, KEY_SLOT, VALUE_SLOT);
	}
	for(int i=0; i < dn->GetChildArrayCount(); i++) {
		const char* key = dn->GetChildArrayKey(i);
		wrenSetSlotString(vm, KEY_SLOT, key);
		WrenHandle* handle_map = wrenGetSlotHandle(vm, MAP_SLOT);
		for(int j=dn->GetArrayLen(key); j >= 0; j--) {  // Minimizes list resizing

			DataNodeToMap(dn->GetChildArrayElem(key, j));
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

int WrenTests() {
	WrenInterface* interface = GetWrenInterface();
	interface->MakeVM();
	interface->LoadWrenQuests();
	if (interface->common_handles.test_class == NULL) {
		ERROR("No 'Tests' class found in 'testing.wren'")
		return 2;
	}
	wrenSetSlotHandle(interface->vm, 0, interface->common_handles.test_class);
	bool success = interface->CallFunc(interface->common_handles.test_call_handle);
	return success ? 0 : 1;
}