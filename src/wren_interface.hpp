#ifndef WREN_INTERFACE_H
#define WREN_INTERFACE_H

#include "basic.hpp"
#include "wren.hpp"
#include <type_traits>

struct WrenQuest {
    WrenHandle* class_handle = NULL;
    char* id;
    int challenge_level;

    WrenQuest() = default;
    ~WrenQuest();

    void Attach(WrenHandle* handle);
};

struct WrenInterface {
    WrenVM* vm;
    WrenQuest* quests = NULL;
    int valid_quest_count = 0;

    WrenInterface();
    ~WrenInterface();

    void MakeVM();
    int LoadWrenQuests();
    WrenQuest* GetWrenQuest(const char* query_id);
    WrenQuest* GetRandomWrenQuest();
    
    bool CallFunc(WrenHandle* func_handle);
    
    bool PrepareMap(const char* key);
    double GetNumFromMap(const char* key, double def);
    bool GetBoolFromMap(const char* key, bool def);
    const char* GetStringFromMap(const char* key, const char* def);
};

WrenVM* GetWrenVM();

#endif  // WREN_INTERFACE_H
