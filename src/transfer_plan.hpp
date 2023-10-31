#ifndef TRANSFER_PLAN_H
#define TRANSFER_PLAN_H

#include "basic.hpp"
#include "planet.hpp"


struct TransferPlan {
    ResourceTransfer resource_transfer;

    // Inputs
    entity_id_t departure_planet;
    entity_id_t arrival_planet;
    time_type departure_time;
    time_type arrival_time;

    // Outputs
    int num_solutions;
    int primary_solution;
    Orbit transfer_orbit[2];
    Vector2 departure_dvs[2];
    Vector2 arrival_dvs[2];
    double dv1[2];
    double dv2[2];
    double tot_dv;
    double tot_dv_sec;
};

struct TransferPlanUI {
    TransferPlan plan;
    entity_id_t ship;
    int resource_type;
    bool is_valid;

    // UI
    bool is_dragging_departure;
    bool is_dragging_arrival;
    Vector2 departure_handle_pos;
    Vector2 arrival_handle_pos;
    bool redraw_queued;
};

void TransferPlanSolve(TransferPlan* tp);

void TransferPlanUIMake(TransferPlanUI* ui);
void TransferPlanUIUpdate(TransferPlanUI* ui);
void TransferPlanUIDraw(TransferPlanUI* ui, const CoordinateTransform* c_transf);

void TransferPlanUISetShip(TransferPlanUI* ui, entity_id_t ship);
void TransferPlanUISetResourceType(TransferPlanUI* ui, int resource_type);
void TransferPlanUISetPayloadMass(TransferPlanUI* ui, resource_count_t payload);
void TransferPlanUISetDestination(TransferPlanUI* ui, entity_id_t planet);
bool TransferPlanUIIsActive(TransferPlanUI* ui);

int TransferPlanTests();

#endif  // TRANSFER_PLAN_H