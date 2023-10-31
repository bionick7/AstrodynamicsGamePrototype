#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"


#define SHIP_MAX_PREPARED_PLANS 10

struct Ship {
    // Inherent properties
    char name[100];
    double max_dv;
    double v_e;
    resource_count_t max_capacity;

    // Current state
    bool is_parked;
    OrbitPos position;
    entity_id_t parent_planet;
    int index_on_planet;

    int prepared_plans_count;
    int confirmed_plans_count;
    TransferPlan prepared_plans[SHIP_MAX_PREPARED_PLANS];

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
    void Update();
    void Draw(const CoordinateTransform* c_transf) const;
    void DrawUI(const CoordinateTransform* c_transf);
    void Inspect();

    TransferPlan* NewTransferPlan();
    TransferPlan* GetEditedTransferPlan();
    void ConfirmEditedTransferPlan();
    void CloseEditedTransferPlan();
    void PopTransferPlan(int index);

private:
    void _OnDeparture(const TransferPlan &tp);
    void _OnArrival(const TransferPlan &tp);
    void _EnsureContinuity();
    void _OnClicked();
};

#endif  // SHIP_H