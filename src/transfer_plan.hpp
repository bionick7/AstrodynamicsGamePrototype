#ifndef TRANSFER_PLAN_H
#define TRANSFER_PLAN_H

#include "basic.hpp"
#include "planet.hpp"
#include "time.hpp"

struct TransferPlan {
    ResourceTransfer resource_transfer;
    resource_count_t fuel_mass;

    // Inputs
    entity_id_t departure_planet;
    entity_id_t arrival_planet;
    Time departure_time;
    Time arrival_time;

    // Outputs
    int num_solutions;
    Orbit transfer_orbit[2];
    Vector2 departure_dvs[2];
    Vector2 arrival_dvs[2];
    double dv1[2];
    double dv2[2];
    double tot_dv;
    double tot_dv_sec;
    
    int primary_solution;

    // Cached
    Time hohmann_departure_time;
    Time hohmann_arrival_time;

    TransferPlan();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
};

void TransferPlanSolve(TransferPlan* tp);
int TransferPlanTests();

struct TransferPlanUI {
    TransferPlan* plan;
    entity_id_t ship;
    int resource_type;
    bool is_valid;

    // UI
    bool is_dragging_departure;
    bool is_dragging_arrival;
    Vector2 departure_handle_pos;
    Vector2 arrival_handle_pos;
    bool redraw_queued;

    Time time_bounds[2];

    TransferPlanUI() { Make(); }
    void Make();
    void Abort();
    void Update();
    void Draw(const CoordinateTransform* c_transf);
    void DrawUI();
    void SetPlan(TransferPlan* plan, entity_id_t ship, Time min_time, Time pos_time);
    void SetResourceType(ResourceType resource_type);
    void SetLogistics(resource_count_t payload_mass, resource_count_t fuel_mass);
    void SetDestination(entity_id_t planet);

    bool IsSelectingDestination();
    bool IsActive();
};


#endif  // TRANSFER_PLAN_H