#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "id_allocator.hpp"
#include "task.hpp"
#include "ship_modules.hpp"

#define SHIP_MAX_PREPARED_PLANS 10
#define SHIP_MAX_MODULES 16

#define SHIPCLASS_NAME_MAX_SIZE 64
#define SHIPCLASS_DESCRIPTION_MAX_SIZE 1024
#define SHIP_NAME_MAX_SIZE 64

struct WrenHandle;

// Pseudo enum to use flags
struct IntelLevel {
    typedef uint32_t Type;
    static const Type TRAJECTORY = 1;
    static const Type STATS = 2;
    static const Type FULL = UINT32_MAX;
};

struct ShipClass {
    char name[SHIPCLASS_NAME_MAX_SIZE];
    char description[SHIPCLASS_DESCRIPTION_MAX_SIZE];
    const char* id = "INVALID ID - SHIP CLASS LOADING ERROR";

    double max_dv;  // m/s
    double v_e;     // m/s
    resource_count_t max_capacity;  // counts
    int stats[ShipStats::MAX] = {0};
    int construction_time;
    resource_count_t build_resources[RESOURCE_MAX] = {0};
    int build_batch_size;

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
    // stats are more or less constat for the same ammount of modules
    int stats[ShipStats::MAX];
    // variables vary
    int dammage_taken[ShipVariables::MAX];

    // Allows to refer to all the stats as variables.
    // Still needs to be declared
    //#define X(upper, lower) const int* lower = &dammage_taken[ShipStats::upper];
    #define X(upper, lower) /*Auto-generated*/ int lower() const { return stats[ShipStats::upper]; };
    X_SHIP_STATS
    #undef X
    
    int plan_edit_index;
    int highlighted_plan_index;

    // UI/visual state
    Vector2 draw_pos;
    bool mouse_hover;
    int index_on_planet;
    int total_on_planet;
    int ui_scroll = 0;
    //Color color;

    // Identifier
    RID id;

    // transporting
    ResourceTransfer transporing;
    ShipModuleSlot current_slot;

    Ship();
    ~Ship();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    
    bool HasMouseHover(double* min_distance) const;
    void Update();
    void _UpdateShipyard();
    void _UpdateModules();
    void Draw(const CoordinateTransform* c_transf) const;
    void DrawUI();
    void Inspect();

    void RemoveShipModuleAt(int index);
    void Repair(int hp);

    double GetPayloadMass() const;
    resource_count_t GetMaxCapacity() const;
    resource_count_t GetRemainingPayloadCapacity(double dv) const;
    resource_count_t GetFuelRequiredEmpty(double dv) const;
    double GetCapableDV() const;
    bool IsPlayerFriend() const;
    IntelLevel::Type GetIntelLevel() const;
    bool IsTrajectoryKnown(int index) const;
    int GetCombatStrength() const;
    int GetMissingHealth() const;
    bool CanDragModule(int index) const;

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

    IDAllocatorList<Ship, EntityType::SHIP> alloc;
    std::map<std::string, RID> ship_classes_ids;
    ShipClass* ship_classes;
    uint32_t ship_classes_count;

    Ships();
    RID AddShip(const DataNode* data);
    int LoadShipClasses(const DataNode* data);
    void ClearShips();

    Ship* GetShip(RID uuid) const;
    RID GetShipClassIndexById(const char* id) const;
    const ShipClass* GetShipClassByIndex(RID index) const;
    void GetOnPlanet(IDList* list, RID planet, uint32_t allegiance_bits) const;
    void KillShip(RID uuid, bool notify_callback);

};

Ship* GetShip(RID uuid);
const ShipClass* GetShipClassByIndex(RID index);
int LoadShipClasses(const DataNode* data);

#endif  // SHIP_H