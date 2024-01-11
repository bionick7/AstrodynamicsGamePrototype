#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "id_allocator.hpp"
#include "task.hpp"
#include "ship_modules.hpp"

#define SHIP_MAX_PREPARED_PLANS 10
#define SHIP_MAX_MODULES 10

#define SHIPCLASS_NAME_MAX_SIZE 64
#define SHIPCLASS_DESCRIPTION_MAX_SIZE 1024
#define SHIP_NAME_MAX_SIZE 64

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

void ShipBattle(const IDList* ships_1, const IDList* ships_2, double relative_velocity);

struct Ship {
    // Inherent properties
    char name[SHIP_NAME_MAX_SIZE];
    RID ship_class;
    int allegiance;  // currently 0 (player) or 1, can swap with RID

    // Current state
    bool is_parked;
    bool is_detected;  // by the other faction
    OrbitPos position;
    RID parent_planet;

    int prepared_plans_count;
    TransferPlan prepared_plans[SHIP_MAX_PREPARED_PLANS];
    RID modules[SHIP_MAX_MODULES];
    int stats[ShipStats::MAX];
    
    int plan_edit_index;
    int highlighted_plan_index;

    // UI/visual state
    Vector2 draw_pos;
    bool mouse_hover;
    int index_on_planet;
    int total_on_planet;
    //Color color;

    // Identifier
    RID id;

    // transporting
    ResourceTransfer transporing;
    ShipModuleSlot current_slot;

    Ship();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    
    bool HasMouseHover(double* min_distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf) const;
    void DrawUI();
    void Inspect();

    void RemoveShipModuleAt(int index);

    double GetPayloadMass() const;
    resource_count_t GetMaxCapacity() const;
    resource_count_t GetRemainingPayloadCapacity(double dv) const;
    resource_count_t GetFuelRequiredEmpty(double dv) const;
    double GetCapableDV() const;
    bool IsPlayerFriend() const;
    bool IsPlayerKnown() const;
    bool IsTrajectoryKnown(int index) const;
    int GetCombatStrength() const;

    Color GetColor() const;

    TransferPlan* GetEditedTransferPlan();
    void ConfirmEditedTransferPlan();
    void CloseEditedTransferPlan();
    void RemoveTransferPlan(int index);
    void StartEditingPlan(int index);

    void _OnDeparture(const TransferPlan &tp);
    void _OnArrival(const TransferPlan &tp);
    void _EnsureContinuity();
    void _OnNewPlanClicked();
    void _OnClicked();
};

struct Ships {
    const static uint32_t MILITARY_SELECTION_FLAG = 0x0100;  // Allowing for 8 factions
    const static uint32_t CIVILIAN_SELECTION_FLAG = 0x0200;

    Ships();
    RID AddShip(const DataNode* data);
    int LoadShipClasses(const DataNode* data);
    void ClearShips();

    Ship* GetShip(RID uuid) const;
    RID GetShipClassIndexById(const char* id) const;
    const ShipClass* GetShipClassByIndex(RID index) const;
    void GetOnPlanet(IDList* list, RID planet, uint32_t allegiance_bits) const;

    IDAllocatorList<Ship, EntityType::SHIP> alloc;
    std::map<std::string, RID> ship_classes_ids;
    ShipClass* ship_classes;
    uint32_t ship_classes_count;
};

Ship* GetShip(RID uuid);
const ShipClass* GetShipClassByIndex(RID index);
int LoadShipClasses(const DataNode* data);

#endif  // SHIP_H