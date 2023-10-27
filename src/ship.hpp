#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"

enum ShipState {
    SHIP_STATE_REST,
    SHIP_STATE_PREPARE_TRANSFER,
    SHIP_STATE_IN_TRANSFER,
};

class Ship {
public:
    // Inherent properties
    char name[100];
    double max_dv;
    double v_e;
    resource_count_t max_capacity;

    // Current state
    ShipState current_state;
    OrbitPos position;
    entity_id_t parent_planet;
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
    entity_id_t id;

    void Make(const char* name);
    double GetPayloadCapacity(double dv) const;
    bool HasMouseHover(double* min_distance) const;
    void AssignTransfer(TransferPlan tp);
    void Update();
    void Draw(const DrawCamera* camera) const;
    void DrawUI(const DrawCamera* camera);
    void Inspect();

    void _OnClicked();
};


#endif  // SHIP_H