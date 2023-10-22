#ifndef SHIP_H
#define SHIP_H

#include "planet.h"
#include "transfer_plan.h"

typedef enum ShipState {
    SHIP_STATE_REST,
    SHIP_STATE_PREPARE_TRANSFER,
    SHIP_STATE_IN_TRANSFER,
} ShipState;

typedef struct Ship {
    const char* name;
    double dv;
    double max_dv;

    ShipState current_state;
    OrbitPos position;

    int parent_planet;
    TransferPlan next_plan;

    int id;
} Ship;

void ShipAssignTransfer(Ship* ship, TransferPlan tp);
void ShipUpdate(Ship* ship);
void ShipDraw(Ship* ship, const DrawCamera* camera, int index_on_planet);
void ShipInspect(const Ship* ship);

#endif  // SHIP_H