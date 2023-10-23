#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.h"
#include "ship.h"
#include "transfer_plan.h"
#include "camera.h"

#define MAX_PLANETS 32
#define MAX_SHIPS 32
#define MAX_CLICKABLE_OBJECTS 32

typedef enum AgentType{
    TYPE_NONE,
    TYPE_SHIP,
    TYPE_PLANET,
} AgentType;

typedef struct Clickable {
    AgentType type;
    int id;
} Clickable;

typedef struct GlobalState {
    time_type time;
    DrawCamera camera;

    int planet_count;
    Planet planets[MAX_PLANETS];
    int ship_count;
    Ship ships[MAX_SHIPS];
    int clickable_count;

    TransferPlanUI active_transfer_plan;
} GlobalState;

GlobalState* GlobalGetState();
time_type GlobalGetNow();

Ship* GetShip(int);

#define GetPlanet(id) _GetPlanet(id, __FILE__, __LINE__)
Planet* _GetPlanet(int id, const char* file, int line);

// Lifecycle
void GlobalStateMake(GlobalState* gs, time_type time);
void LoadGlobalState(GlobalState* gs, const char* file_path);
void DestroyGlobalState(GlobalState* gs);

// Update
void UpdateState(GlobalState* gs, double delta_t);

// Draw
void DrawState(GlobalState* gs);

#endif // GLOBAL_STATE_H