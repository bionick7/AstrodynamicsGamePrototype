#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"
#include "quests.hpp"

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
    CoordinateTransform c_transf;
    Calendar calendar;
    TransferPlanUI active_transfer_plan;
    QuestManager quest_manager;
    Ships ships;
    Planets planets;

    entity_id_t focused_planet;
    entity_id_t focused_ship;

    int capital;

    // Lifecycle
    void Make(Time time);
    void LoadData();
    void LoadGame(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    // Interaction
    bool CompleteTransaction(int delta, const char* message);

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    //entt::registry registry;
    void _InspectState();
};

GlobalState* GlobalGetState();
Time GlobalGetNow();
Time GlobalGetPreviousFrameTime();
Planet* GetPlanetByName(const char* planet_name);

#endif // GLOBAL_STATE_H
