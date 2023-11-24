#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"

#define SHIP_MAX_PREPARED_PLANS 10
#define SHIPCLASS_NAME_MAX_SIZE 64
#define SHIPCLASS_DESCRIPTION_MAX_SIZE 1024
#define SHIP_NAME_MAX_SIZE 64

typedef uint16_t shipclass_index_t;

struct ShipClass {
    char name[SHIPCLASS_NAME_MAX_SIZE];
    char description[SHIPCLASS_DESCRIPTION_MAX_SIZE];
    const char* id = "INVALID ID - SHIP CLASS LOADING ERROR";

    double max_dv;
    double v_e;
    resource_count_t max_capacity;

    double GetPayloadCapacity(double dv) const;
    double GetFuelRequiredEmpty(double dv) const;
    double GetFuelRequiredFull(double dv) const;
};

int LoadShipClasses(const DataNode* data);
shipclass_index_t GetShipClassIndexById(const char* id);
const ShipClass* GetShipClassByIndex(shipclass_index_t index);

struct Ship {
    // Inherent properties
    char name[SHIP_NAME_MAX_SIZE];
    shipclass_index_t ship_class;
    //const ShipClass* ship_class;

    // Current state
    bool is_parked;
    OrbitPos position;
    entity_id_t parent_planet;
    int index_on_planet;

    int prepared_plans_count;
    TransferPlan prepared_plans[SHIP_MAX_PREPARED_PLANS];
    int plan_edit_index;
    int highlighted_plan_index;

    // Payload
    int payload_type = 0;
    resource_count_t payload_quantity = 0;

    // UI state
    Vector2 draw_pos;
    bool mouse_hover;
    Color color;

    // Identifier
    entity_id_t id;

    void CreateFrom(const ShipClass* sc);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    
    bool HasMouseHover(double* min_distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf) const;
    void DrawUI(const CoordinateTransform* c_transf);
    void Inspect();

    TransferPlan* GetEditedTransferPlan();
    void ConfirmEditedTransferPlan();
    void CloseEditedTransferPlan();
    void RemoveTransferPlan(int index);
    void StartEditingPlan(int index);

private:
    TransferPlan* _NewTransferPlan();
    void _OnDeparture(const TransferPlan &tp);
    void _OnArrival(const TransferPlan &tp);
    void _EnsureContinuity();
    void _OnNewPlanClicked();
    void _OnClicked();
};

#endif  // SHIP_H