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
    int valid_quest_count = 0;
    WrenQuestTemplate* quests = NULL;

    WrenHandle* internals_class_handle = NULL;

    WrenInterface();
    ~WrenInterface();

    void MakeVM();
    int LoadWrenQuests();

    WrenQuestTemplate* GetWrenQuest(const char* query_id) const;
    WrenQuestTemplate* GetRandomWrenQuest() const;
    
    bool CallFunc(WrenHandle* func_handle) const;
    void MoveSlot(int from, int to) const;

    bool PrepareMap(const char* key) const;
    double GetNumFromMap(const char* key, double def) const;
    bool GetBoolFromMap(const char* key, bool def) const;
    const char* GetStringFromMap(const char* key, const char* def) const;
    void _MapAsDataNodePopulateList(DataNode *dn, const char* key) const;
    bool MapAsDataNode(DataNode* dn) const;
    bool DataNodeToMap(const DataNode* dn) const;
};

WrenVM* GetWrenVM();

#endif  // WREN_INTERFACE_H
