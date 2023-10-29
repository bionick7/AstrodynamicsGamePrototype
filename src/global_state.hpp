#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
//#include "resource_allocator.hpp"
#include "camera.hpp"
#include <map>

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
    DrawCamera camera;

/*
    int planet_count;
    Planet planets[MAX_PLANETS];
    int ship_count;
    Ship ships[MAX_SHIPS];
    int clickable_count;
*/
    TransferPlanUI active_transfer_plan;


    // Lifecycle
    void Make(time_type time);
    void Load(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    entt::registry registry;
    entity_id_t _AddPlanet(
        const char* name, double mu_parent, 
        double sma, double ecc, double lop, double ann, bool is_prograde, 
        double radius, double mu
    );
    entity_id_t _AddShip(const char* name, entity_id_t origin_planet);
    void _InspectState();
};

GlobalState* GlobalGetState();
time_type GlobalGetNow();
time_type GlobalGetPreviousFrameTime();


Ship& GetShip(entity_id_t uuid);
Planet& GetPlanet(entity_id_t uuid);

#endif // GLOBAL_STATE_H