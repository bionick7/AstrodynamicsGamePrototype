#ifndef TRANSFER_PLAN_H
#define TRANSFER_PLAN_H

#include "basic.hpp"
#include "planet.hpp"
#include "time.hpp"

struct TransferPlan {
    resource_count_t resource_transfer[resources::MAX];
    resources::T fuel_type;
    resource_count_t fuel;

    // Inputs
    RID departure_planet;
    RID arrival_planet;
    timemath::Time departure_time;
    timemath::Time arrival_time;

    // Outputs
    int num_solutions;
    Orbit transfer_orbit[2];
    DVector3 departure_dvs[2];
    DVector3 arrival_dvs[2];
    double dv1[2];
    double dv2[2];
    double tot_dv;
    double tot_dv_sec;
    
    int primary_solution;

    // Cached
    timemath::Time hohmann_departure_time;
    timemath::Time hohmann_arrival_time;

    TransferPlan();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    resource_count_t GetPayloadMass() const;
};

void TransferPlanSolve(TransferPlan* tp);
void TransferPlanSetBestDeparture(TransferPlan* tp, timemath::Time t0, timemath::Time t1);
void TransferPlanSoonest(TransferPlan* tp, double dv_limit, timemath::Time earliest);
int TransferPlanTests();

struct TransferPlanCycle {
    int stops = 0;
    RID* planets = NULL;
    double* dvs = NULL;
    resource_count_t (*resource_transfers)[resources::MAX] = NULL;

    ~TransferPlanCycle();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void GenFromTransferplans(TransferPlan* transferplans, int transferplan_count);
    void Reset();
};

struct TransferPlanUI {
    TransferPlan* plan;
    RID ship;
    bool is_valid;

    // UI
    bool is_dragging_departure;
    bool is_dragging_arrival;
    Vector2 departure_handle_pos;
    Vector2 arrival_handle_pos;
    bool redraw_queued;

    bool departure_time_automatic;

    timemath::Time time_bounds[2];

    // 3d
    RID orbit_renders[2];

    TransferPlanUI();
    void Reset();
    void Abort();
    void Update();
    void Draw3D();
    void Draw3DGizmos();
    void DrawUI();
    void SetPlan(TransferPlan* plan, RID ship, timemath::Time min_time, timemath::Time pos_time);
    void SetLogistics(resource_count_t fuel_mass);
    void SetDestination(RID planet);

    bool IsSelectingDestination();
    bool IsActive();
};


#endif  // TRANSFER_PLAN_H