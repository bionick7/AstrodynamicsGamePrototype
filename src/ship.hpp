#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "id_allocator.hpp"
#include "quests.hpp"

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

    // Ease-of-use variables
    double oem;  // kg

    double GetPayloadCapacityMass(double dv) const;
    resource_count_t GetFuelRequiredEmpty(double dv) const;
    resource_count_t GetFuelRequiredFull(double dv) const;
};

struct Ship {
    // Inherent properties
    char name[SHIP_NAME_MAX_SIZE];
    shipclass_index_t ship_class;

    // Current state
    bool is_parked;
    OrbitPos position;
    entity_id_t parent_planet;
    int index_on_planet;

    int prepared_plans_count;
    TransferPlan prepared_plans[SHIP_MAX_PREPARED_PLANS];
    int plan_edit_index;
    int highlighted_plan_index;

    // UI state
    Vector2 draw_pos;
    bool mouse_hover;
    Color color;

    // Identifier
    entity_id_t id;

    // transporting
    ResourceTransfer transporing;

    void CreateFrom(const ShipClass* sc);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    
    bool HasMouseHover(double* min_distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf) const;
    void DrawUI(const CoordinateTransform* c_transf);
    void Inspect();

    double GetPayloadMass() const;
    resource_count_t GetMaxCapacity() const;
    resource_count_t GetRemainingPayloadCapacity(double dv) const;
    resource_count_t GetFuelRequiredEmpty(double dv) const;
    double GetCapableDV() const;

    TransferPlan* GetEditedTransferPlan();
    void ConfirmEditedTransferPlan();
    void CloseEditedTransferPlan();
    void RemoveTransferPlan(int index);
    void StartEditingPlan(int index);
    TransferPlan* _NewTransferPlan();

    void _OnDeparture(const TransferPlan &tp);
    void _OnArrival(const TransferPlan &tp);
    void _EnsureContinuity();
    void _OnNewPlanClicked();
    void _OnClicked();
};

struct Ships {
    Ships();
    entity_id_t AddShip(const DataNode* data);
    int LoadShipClasses(const DataNode* data);
    void ClearShips();

    Ship* GetShip(entity_id_t uuid) const;
    shipclass_index_t GetShipClassIndexById(const char* id) const;
    const ShipClass* GetShipClassByIndex(shipclass_index_t index) const;

    IDAllocatorList<Ship> alloc;
private:
    std::map<std::string, shipclass_index_t> ship_classes_ids;
    ShipClass* ship_classes;
    size_t ship_classes_count;
};

Ship* GetShip(entity_id_t uuid);
const ShipClass* GetShipClassByIndex(shipclass_index_t index);
int LoadShipClasses(const DataNode* data);

#endif  // SHIP_H