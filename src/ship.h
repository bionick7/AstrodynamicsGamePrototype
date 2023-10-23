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
    char name[100];
    double dv;
    double max_dv;

    ShipState current_state;
    OrbitPos position;

    int parent_planet;
    int index_on_planet;
    TransferPlan next_plan;
    Vector2 draw_pos;

    bool mouse_hover;
    int id;
} Ship;

bool ShipHasMouseHover(const Ship* ship, double* min_distance);
void ShipAssignTransfer(Ship* ship, TransferPlan tp);
void ShipUpdate(Ship* ship);
void ShipDraw(Ship* ship, const DrawCamera* camera);
void ShipInspect(const Ship* ship);

#endif  // SHIP_H