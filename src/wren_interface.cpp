#include "wren_interface.hpp"
#include "logging.hpp"
#include "core/string_builder.hpp"

WrenInterface::WrenInterface() {

}

WrenInterface::~WrenInterface() {

}

void QuestInterfaceRequireTransport(WrenVM* vm) {

}

void QuestInterfaceWaitSeconds(WrenVM* vm) {

}

void QuestInterfaceIsTaskImpossible(WrenVM* vm) {

}

void QuestInterfaceInvalidTask(WrenVM* vm) {

}

void QuestInterfacePayMoney(WrenVM* vm) {

}

void QuestInterfacePayItem(WrenVM* vm) {

}

void QuestInterfacePayReputation(WrenVM* vm) {

}

static void WriteFn(WrenVM* vm, const char* text) {
	if (strcmp(text, "\n") != 0) {
        LogImpl("unknown", 0, LOGTYPE_WRENINFO, text);
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
    LogImpl(" - ", 0, LOGTYPE_WRENERROR, "Tried to call non implented foreign function");
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
	if (isStatic) {
        if (strcmp(className, "Quest") == 0) {
            if (strcmp(signature, "require_transport()") == 0) {
                return &QuestInterfaceRequireTransport;
            } else if (strcmp(signature, "wait_seconds()") == 0) {
                return &QuestInterfaceWaitSeconds;
            } else if (strcmp(signature, "is_task_possible(_)") == 0) {
                return &QuestInterfaceIsTaskImpossible;
            } else if (strcmp(signature, "invalid_task") == 0) {
                return &QuestInterfaceInvalidTask;
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
	}

    ERROR("Only static functions supported")
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

void WrenInterface::LoadQuests() {
	FilePathList fps = LoadDirectoryFilesEx("resources/wren/quests", ".wren", true);
	for (int i=0; i < fps.count; i++) {
		const char* name = GetFileNameWithoutExt(fps.paths[i]);
		INFO("Loading %s", name);
		wrenInterpret(vm, name, LoadFileText(fps.paths[i]));
	}
	UnloadDirectoryFiles(fps);

	//wrenEnsureSlots(vm, 1);
	//INFO("start")
	//wrenGetVariable(vm, "quests/example", "ExampleQuest", 0);
	//INFO("end")
	//WrenHandle* example_quest_class = wrenGetSlotHandle(vm, 0);
}

// resources wren foreign quest_base.wren
// resources wren foreign quest_base.wren