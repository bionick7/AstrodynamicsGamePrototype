#ifndef TRANSFER_PLAN_H
#define TRANSFER_PLAN_H

#include "basic.hpp"
#include "planet.hpp"


STRUCT_DECL(TransferPlan) {
    ResourceTransfer resource_transfer;

    // Inputs
    int departure_planet;
    int arrival_planet;
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

STRUCT_DECL(TransferPlanUI) {
    TransferPlan plan;
    int ship;
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
void TransferPlanUIDraw(TransferPlanUI* ui, const DrawCamera* cam);

void TransferPlanUISetShip(TransferPlanUI* ui, int ship);
void TransferPlanUISetResourceType(TransferPlanUI* ui, int resource_type);
void TransferPlanUISetPayloadMass(TransferPlanUI* ui, resource_count_t payload);
void TransferPlanUISetDestination(TransferPlanUI* ui, int planet);
bool TransferPlanUIIsActive(TransferPlanUI* ui);

#endif  // TRANSFER_PLAN_H