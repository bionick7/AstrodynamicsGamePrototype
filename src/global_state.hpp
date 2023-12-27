#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"
#include "quests.hpp"
#include "audio_server.hpp"
#include "wren_interface.hpp"

enum AgentType {
    TYPE_NONE,
    TYPE_SHIP,
    TYPE_PLANET,
};

struct Clickable {
    AgentType type;
    entity_id_t id;
};

bool IsIdValid(entity_id_t id);
constexpr entity_id_t GetInvalidId() { return UINT32_MAX; }

struct GlobalState {
    enum FocusablesPanels {
        QUEST_MANAGER,
        TIMELINE,
        BUILDING_CONSTRUCTION,
        TRANSFER_PLAN_UI,
        PLANET_SHIP_DETAILS,
        MAP
    };

    FocusablesPanels current_focus;

    CoordinateTransform c_transf;
    Calendar calendar;
    TransferPlanUI active_transfer_plan;
    QuestManager quest_manager;
    Ships ships;
    Planets planets;

    AudioServer audio_server;
    WrenInterface wren_interface;
    Font default_font;
    
    entity_id_t focused_planet;
    entity_id_t focused_ship;

    cost_t capital;

    // Lifecycle
    void Make(timemath::Time time);
    void LoadData();
    void LoadGame(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    // Interaction
    FocusablesPanels GetCurrentFocus();
    bool CompleteTransaction(int delta, const char* message);

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    //entt::registry registry;
    void _InspectState();
};

GlobalState* GlobalGetState();
timemath::Time GlobalGetNow();
AudioServer* GetAudioServer();

#endif // GLOBAL_STATE_H
