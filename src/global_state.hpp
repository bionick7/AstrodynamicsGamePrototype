#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"

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
    Time time;
    Time prev_time;
    CoordinateTransform c_transf;
    TransferPlanUI active_transfer_plan;
    entity_id_t focused_planet;
    entity_id_t focused_ship;

    // Everything that doesn't fit anywhere else
    double parent_radius;
    double parent_mu;

    // Lifecycle
    void Make(Time time);
    void LoadConfigs(const char* ephemerides_path, const char* module_data_path);
    void LoadGame(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    entt::registry registry;
    void _InspectState();
};

GlobalState* GlobalGetState();
Time GlobalGetNow();
Time GlobalGetPreviousFrameTime();
Planet* GetPlanetByName(const char* planet_name);

#define GetPlanet(uuid) _GetPlanet(uuid, __FILE__, __LINE__)
Ship& GetShip(entity_id_t uuid);
Planet& _GetPlanet(entity_id_t uuid, const char* file, int line);

#endif // GLOBAL_STATE_H