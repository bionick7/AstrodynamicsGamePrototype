#ifndef WREN_INTERFACE_H
#define WREN_INTERFACE_H

#include "basic.hpp"
#include "wren.hpp"
#include "datanode.hpp"
#include "id_system.hpp"

#include <queue>

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
    std::queue<const WrenQuestTemplate*> quest_queue = std::queue<const WrenQuestTemplate*>();

    WrenHandle* internals_class_handle = NULL;
    struct {
        // More handles here
        WrenHandle* quest_notify;

        WrenHandle* END_HANDLE = NULL;  // to access raw pointer
    } common_handles;

    WrenInterface();
    ~WrenInterface();

    void MakeVM();
    int LoadWrenQuests();

    WrenQuestTemplate* GetWrenQuest(const char* query_id) const;
    WrenQuestTemplate* GetRandomWrenQuest() const;
    
    bool CallFunc(WrenHandle* func_handle) const;
    void MoveSlot(int from, int to) const;
    RID GetShipFormWrenObject() const;
    void NotifyShipEvent(RID ship, const char* event);
    void PushQuest(const char* quest_id);

    void Update();

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
