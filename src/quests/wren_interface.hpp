#ifndef WREN_INTERFACE_H
#define WREN_INTERFACE_H

#include "basic.hpp"
#include "wren.hpp"
#include "datanode.hpp"
#include <type_traits>

struct WrenQuestTemplate {
    WrenHandle* class_handle = NULL;
    char* id;
    int challenge_level;

    WrenQuestTemplate() = default;
    ~WrenQuestTemplate();

    void Attach(WrenHandle* handle);
};

struct WrenInterface {
    WrenVM* vm;
    WrenQuestTemplate* quests = NULL;
    int valid_quest_count = 0;

    WrenHandle* internals_class_handle = NULL;

    WrenInterface();
    ~WrenInterface();

    void MakeVM();
    int LoadWrenQuests();
    WrenQuestTemplate* GetWrenQuest(const char* query_id);
    WrenQuestTemplate* GetRandomWrenQuest();
    
    bool CallFunc(WrenHandle* func_handle);
    
    bool PrepareMap(const char* key);
    double GetNumFromMap(const char* key, double def);
    bool GetBoolFromMap(const char* key, bool def);
    const char* GetStringFromMap(const char* key, const char* def);
    void _DictAsDataNodePopulateList(DataNode *dn, const char* key);
    bool DictAsDataNode(DataNode* dn);
};

WrenVM* GetWrenVM();

#endif  // WREN_INTERFACE_H
