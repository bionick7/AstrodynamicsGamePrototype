#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "planet.h"
#include "transfer_plan.h"
#include "camera.h"

#define MAX_PLANETS 32
#define MAX_SHIPS 32

typedef struct Ship {
    Orbit orbit;
    Vector2 position;
    Vector2 velocity;
} Ship;

typedef struct GlobalState {
    time_type time;
    DrawCamera camera;

    int planet_count;
    Planet planets[MAX_PLANETS];
    int ship_count;
    Ship ships[MAX_SHIPS];
    TransferPlan active_transfer_plan;
} GlobalState;

GlobalState* GetGlobalState();

// Lifecycle
void MakeGlobalState(GlobalState* gs, time_type time);
void LoadGlobalState(GlobalState* gs, const char* file_path);
void DestroyGlobalState(GlobalState* gs);

// Update
void UpdateState(GlobalState* gs, double delta_t);

// Draw
void DrawState(GlobalState* gs);

#endif // GLOBAL_STATE_H