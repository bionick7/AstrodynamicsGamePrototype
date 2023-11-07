#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"

#define MAX_PLANETS 32
#define MAX_SHIPS 32
#define MAX_CLICKABLE_OBJECTS 32

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
static inline entity_id_t GetInvalidId() { return entt::null; }

struct GlobalState {
    time_type time;
    time_type prev_time;
    CoordinateTransform c_transf;
    TransferPlanUI active_transfer_plan;
    entity_id_t focused_planet;
    entity_id_t focused_ship;

    // Lifecycle
    void Make(time_type time);
    void Load(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    entt::registry registry;
    entity_id_t _AddShip(int index, entity_id_t origin_planet);
    void _InspectState();
};

GlobalState* GlobalGetState();
time_type GlobalGetNow();
time_type GlobalGetPreviousFrameTime();


Ship& GetShip(entity_id_t uuid);
#define GetPlanet(uuid) _GetPlanet(uuid, __FILE__, __LINE__)
Planet& _GetPlanet(entity_id_t uuid, const char* file, int line);

#endif // GLOBAL_STATE_H