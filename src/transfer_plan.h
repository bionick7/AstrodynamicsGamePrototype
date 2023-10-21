#ifndef TRANSFER_PLAN_H
#define TRANSFER_PLAN_H

#include "basic.h"
#include "planet.h"

typedef enum TransferPlanUIState {
    TPUISTATE_NONE = 0,
    TPUISTATE_MOD_DEPARTURE,
    TPUISTATE_MOD_ARRIVAL,
} TransferPlanUIState;

typedef struct TransferPlan {
    // Inputs
    const Planet* from;
    const Planet* to;
    time_type departure_time;
    time_type arrival_time;

    // Outputs
    Orbit transfer_orbit[4];
    int num_solutions;
    Vector2 v1;
    Vector2 v2;

    // UI
    TransferPlanUIState state;
    Vector2 departure_handle_pos;
    Vector2 arrival_handle_pos;

    // Cached
    Vector2 orbit_buffer[ORBIT_BUFFER_SIZE];
} TransferPlan;

void TransferPlanSolve(TransferPlan* tp);
void TransferPlanUpdate(TransferPlan* tp);
void TransferPlanDraw(TransferPlan* tp, const DrawCamera* cam);
void TransferPlanAddPlanet(TransferPlan* tp, const Planet* planet);

#endif  // TRANSFER_PLAN_H