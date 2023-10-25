#ifndef SHIP_H
#define SHIP_H

#include "planet.h"
#include "transfer_plan.h"

ENUM_DECL(ShipState) {
    SHIP_STATE_REST,
    SHIP_STATE_PREPARE_TRANSFER,
    SHIP_STATE_IN_TRANSFER,
};

STRUCT_DECL(Ship) {
    // Inherent properties
    char name[100];
    double max_dv;
    double v_e;
    resource_count_t max_capacity;

    // Current state
    ShipState current_state;
    OrbitPos position;
    int parent_planet;
    int index_on_planet;
    TransferPlan next_plan;

    // Payload
    int respource_type;
    resource_count_t respource_qtt;

    // UI state
    Vector2 draw_pos;
    bool mouse_hover;
    Color color;

    // Identifier
    int id;
};

void ShipMake(Ship* ship, const char* name);
double ShipGetPayloadCapacity(const Ship* ship, double dv);
bool ShipHasMouseHover(const Ship* ship, double* min_distance);
void ShipAssignTransfer(Ship* ship, TransferPlan tp);
void ShipUpdate(Ship* ship);
void ShipDraw(Ship* ship, const DrawCamera* camera);
void ShipDrawUI(Ship* ship, const DrawCamera* camera);
void ShipInspect(const Ship* ship);

#endif  // SHIP_H