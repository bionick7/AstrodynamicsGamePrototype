#ifndef SHIP_H
#define SHIP_H

#include "planet.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "id_allocator.hpp"
#include "task.hpp"
#include "ship_modules.hpp"

#define SHIP_MAX_PREPARED_PLANS 10

#define SHIPCLASS_NAME_MAX_SIZE 64
#define SHIPCLASS_DESCRIPTION_MAX_SIZE 1024
#define SHIP_NAME_MAX_SIZE 64

struct WrenHandle;

// Pseudo enum to use flags
namespace intel_level {
    typedef uint32_t T;
    static const T TRAJECTORY = 1;
    static const T STATS = 2;
    static const T FULL = UINT32_MAX;
};

namespace ship_type {
    enum T {
        UTILITY = 0,
        SHIPYARD,
        TRANSPORT,
        MILITARY,
    };
};

struct ShipClass {
    char name[SHIPCLASS_NAME_MAX_SIZE];
    char description[SHIPCLASS_DESCRIPTION_MAX_SIZE];
    const char* id = "INVALID ID - SHIP CLASS LOADING ERROR";

    double max_dv;  // m/s
    double v_e;     // m/s
    resources::T fuel_resource;
    resource_count_t max_capacity;  // counts

    ModuleConfiguration module_config;
    int stats[ship_stats::MAX] = {0};

    int construction_time;
    resource_count_t construction_resources[resources::MAX] = {0};
    int construction_requirements[ship_stats::MAX] = {0};
    int construction_batch_size;
    bool is_hidden;
    
    // Visuals
    AtlasPos icon_index;

    // Ease-of-use variables
    double oem;  // kg

    double GetPayloadCapacityMass(double dv, int drop_tanks) const;
    resource_count_t GetFuelRequiredFull(double dv, int drop_tanks) const;
    resource_count_t GetFuelRequiredEmpty(double dv) const;
    void MouseHintWrite(StringBuilder* sb) const;
};

struct Ship {
    // Inherent properties from ship_class
    char name[SHIP_NAME_MAX_SIZE];
    RID ship_class;
    int allegiance;  // 0-7

    // Current state
    RID parent_obj;

    bool is_detected;  // by the other faction
    OrbitPos position;

    int prepared_plans_count;
    TransferPlan prepared_plans[SHIP_MAX_PREPARED_PLANS];
    RID modules[SHIP_MAX_MODULES];
    // stats are more or less constat for the same ammount of modules
    int stats[ship_stats::MAX];
    // variables vary
    int dammage_taken[ship_variables::MAX];

    // Allows to refer to all the stats as variables.
    // Still needs to be declared
    #define X(upper, lower) int lower() const { return stats[ship_stats::upper]; };
    X_SHIP_STATS
    #undef X
    
    int plan_edit_index;
    int highlighted_plan_index;

    // UI/visual state
    Vector2 draw_pos;
    bool mouse_hover;
    bool ui_submenu_toggles[3] = { false, false, false };

    // 3D references
    RID icon3d;
    RID text3d;

    // Identifier
    RID id;

    // Payload
    resource_count_t transporting[resources::MAX];
    ShipModuleSlot current_slot;

    // Production
    int production_process;
    IDList production_queue;

    Ship();
    ~Ship();

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);
    
    bool HasMouseHover(double* min_distance) const;
    void Update();
    void _UpdateShipyard();
    void _UpdateModules();
    void DrawIcon(int x_offsets[], int y_offsets[], float grow_factor);
    void DrawTrajectories() const;
    void DrawUI();
    void Inspect();
    
    ShipModuleSlot GetFreeModuleSlot(module_types::T least) const;
    void RemoveShipModuleAt(int index);
    void Repair(int hp);
    void AttachTo(RID parent_ship);
    void Detach();

    double GetOperationalMass() const;
    resource_count_t GetMaxCapacity() const;
    resource_count_t GetRemainingPayloadCapacity(double dv) const;
    resource_count_t GetFuelRequiredFull(double dv) const;
    resource_count_t GetFuelRequired(double dv, resource_count_t payload) const;
    double GetCapableDV() const;
    bool IsPlayerControlled() const;
    intel_level::T GetIntelLevel() const;
    bool IsTrajectoryKnown(int index) const;
    int GetCombatStrength() const;
    int GetMissingHealth() const;
    const char* GetTypeIcon() const;

    bool CanDragModule(int index) const;
    bool IsParked() const;
    bool IsLeading() const;
    RID GetParentPlanet() const;
    int CountModulesOfClass(RID module_class) const;
    ship_type::T GetShipType() const;
    Color GetColor() const;
    bool IsStatic() const;
    bool CanProduce() const;

    // Transfer plans
    TransferPlan* GetEditedTransferPlan();
    void ConfirmEditedTransferPlan();
    void CloseEditedTransferPlan();
    void RemoveTransferPlan(int index);
    void StartEditingPlan(int index);

    // Production
    bool CanProduce(RID object, bool check_resources, bool check_stats) const;
    void AdvanceShipProductionQueue();
    void AdvanceModuleProductionQueue();

    void _OnDeparture(const TransferPlan *tp);
    void _OnArrival(const TransferPlan *tp);
    void _EnsureContinuity();
    void _OnNewPlanClicked();
    void _OnClicked();
};

namespace ship_selection_flags {
    typedef uint32_t T;

    const static T MILITARY = 0x0100;  // Allowing for 8 factions
    const static T CIVILIAN = 0x0200;
    const static T ALL_SELECTION  = 0xFFFFFF00;
    const static T ALL_ALLEGIANCE = 0x000000FF;
    const static T ALL = ALL_SELECTION | ALL_ALLEGIANCE;

    constexpr T GetAllegianceFlags(unsigned int index);
    bool MatchesShip(T selectio_flags, const Ship* ship);
}

struct Ships {
    IDAllocatorList<Ship, EntityType::SHIP> alloc;
    ShipClass* ship_classes;
    uint32_t ship_classes_count;

    Ships();
    RID AddShip(const DataNode* data);
    int LoadShipClasses(const DataNode* data);
    void ClearShips();

    Ship* GetShip(RID uuid) const;
    const ShipClass* GetShipClassByRID(RID index) const;
    void GetOnPlanet(IDList* list, RID planet, ship_selection_flags::T selection) const;
    void GetAll(IDList* list, ship_selection_flags::T selection) const;
    void GetFleet(IDList* list, RID ship) const;
    void KillShip(RID uuid, bool notify_callback);
    void DrawShipClassUI(RID uuid) const;
};

Ship* GetShip(RID uuid);
const ShipClass* GetShipClassByRID(RID index);
int LoadShipClasses(const DataNode* data);

#endif  // SHIP_H