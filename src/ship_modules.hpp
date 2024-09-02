#ifndef SHIP_MODULES_H
#define SHIP_MODULES_H

#include "basic.hpp"
#include "datanode.hpp"
#include "id_system.hpp"
#include "planetary_economy.hpp"
#include "ui.hpp"
#include "assets.hpp"

struct Ship;

#define SHIP_MAX_MODULES 16
#define SHIP_MAX_FREE_MODULES 32
#define SHIP_MODULE_WIDTH 50
#define SHIP_MODULE_HEIGHT 50

#define X_SHIP_STATS \
    X(POWER, power)\
    X(INITIATIVE, initiative)\
    X(KINETIC_HP, kinetic_hp)\
    X(ENERGY_HP, energy_hp)\
    X(CREW, crew)\
    X(KINETIC_OFFENSE, kinetic_offense)\
    X(ORDNANCE_OFFENSE, ordnance_offense)\
    X(BOARDING_OFFENSE, boarding_offense)\
    X(KINETIC_DEFENSE, kinetic_defense)\
    X(ORDNANCE_DEFENSE, ordnance_defense)\
    X(BOARDING_DEFENSE, boarding_defense)\
    X(GROUND_CONNECTION, ground_connection)\
    X(THERMAL_CONTROL, thermal_control)\
    X(INDUSTRIAL_ADMIN, industrial_admin)\
    X(INDUSTRIAL_STORAGE, industrial_storage)\
    X(INDUSTRIAL_MANUFACTURING, industrial_manufacturing)\
    X(INDUSTRIAL_DOCK, industrial_dock)\
    X(CRYOGENICS_FACILITY, cryogenics_facility)\
    X(CLEAN_ROOM, clean_room)\
    X(BIO_MANUFACTURING, bio_manufacturing)\
    X(ARMS_MANUFACTURING, arms_manufacturing)\
    X(PRECISION_MANUFACTURING, precision_manufacturing)\
    X(NUCLEAR_ENRICHMENT, nuclear_enrichment)\
    X(MILITARY_TRAINING, military_training)\

namespace ship_stats {  // Better enum class, since you can treat the enum as integer
    enum T {
        #define X(upper, lower) upper,
        X_SHIP_STATS
        #undef X
        MAX,
    };

    static const char* names[] = {
        #define X(upper, lower) #lower,
        X_SHIP_STATS
        #undef X
    };

    static const char* icons[] = {
        #define X(upper, lower) ICON_##upper,
        X_SHIP_STATS
        #undef X
    };
};

namespace ship_variables {
    enum T {
        KINETIC_ARMOR,
        ENERGY_ARMOR,
        CREW,

        MAX,
    };

    static const char* names[] = {
        "kinetic_armor",
        "energy_armor",
        "crew",
    };
};

static_assert(ship_stats::MAX == sizeof(ship_stats::names) / sizeof(ship_stats::names[0]));
static_assert(ship_variables::MAX == sizeof(ship_variables::names) / sizeof(ship_variables::names[0]));

namespace module_types {
    static const char* names[] = {
        "large",
        "medium",
        "small",
        "free",
        "armor",
        "drop_tank",
        "any",
    };
    
    static const char* str_icons[] = {
        ICON_MODULE_TYPE_LARGE,
        ICON_MODULE_TYPE_MEDIUM,
        ICON_MODULE_TYPE_SMALL,
        ICON_MODULE_TYPE_FREE,
        ICON_MODULE_TYPE_ARMOR,
        ICON_MODULE_TYPE_DROP,
        ICON_MODULE_TYPE_ANY,
    };

    static AtlasPos icons[] = {
        {  0, 23},
        {  1, 23},
        {  2, 23},
        {  3, 23},
        {  4, 23},
        {  5, 23},
        {  6, 23},
    };


    enum T {
        INVALID = -1,
        LARGE = 0,
        MEDIUM,
        SMALL,
        FREE,
        ARMOR,
        DROPTANK,
        ANY,

        MAX,
    };
    static_assert(sizeof(names) / sizeof(char*) == MAX);
    static_assert(sizeof(str_icons) / sizeof(char*) == MAX);
    static_assert(sizeof(icons) / sizeof(AtlasPos) == MAX);

    T FromString(const char* name);
    bool IsCompatible(T from, T to);
};

#define MODULE_CONFIG_MAX_NEIGHBOURS 4

struct ShipModuleClass {
    int delta_stats[ship_stats::MAX];
    int required_stats[ship_stats::MAX];
    int construction_requirements[ship_stats::MAX];
    resource_count_t production[resources::MAX];
    resource_count_t construction_resources[resources::MAX];
    int independence_delta;
    int opinion_delta;
    int construction_batch_size;
    uint64_t planets_restriction;

    double mass;  // kg
    char name[100];
    char description[512];
    bool has_stat_dependencies;
    int construction_time;
    bool is_hidden;
    module_types::T type;

    AtlasPos icon_index;

    ShipModuleClass();
    void MouseHintWrite(StringBuilder* sb) const;
    bool IsPlanetRestricted(RID planet) const;
    void UpdateCustom(Ship* ship) const;
    bool HasStatDependencies() const;
    int GetConstructionTime() const;
    const char* id;
};

struct ShipModuleSlot {
    enum ShipModuleSlotType { DRAGGING_FROM_SHIP, DRAGGING_FROM_PLANET };
    RID entity = GetInvalidId();
    int index = -1;
    ShipModuleSlotType origin_type;
    module_types::T module_type;

    ShipModuleSlot() = default;
    ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_origin_type, module_types::T p_module_type);

    void SetSlot(RID module) const;
    RID GetSlot() const;
    void AssignIfValid(ShipModuleSlot other);
    bool IsValid() const;
    bool IsReachable(ShipModuleSlot other) const;
    bool IsSlotFitting(RID module) const;

    void Draw() const;
};

struct ModuleConfiguration {
    // Essential data
    int module_count = 0;
    module_types::T types[SHIP_MAX_MODULES];
    int neighbours[SHIP_MAX_MODULES*MODULE_CONFIG_MAX_NEIGHBOURS];
    Vector2 draw_offset[SHIP_MAX_MODULES];

    // Rendering cached
    Rectangle mesh_draw_space;
    char mesh_resource_path[ASSET_PATH_MAX_LENGTH];

    void Load(const DataNode* data, const char* ship_id);
    void Draw(Ship* ship, Vector2 anchor_point, text_alignment::T alignment) const;
    bool IsAdjacent(ShipModuleSlot lhs, ShipModuleSlot rhs) const;
};

struct ShipModules {
    ShipModuleClass* ship_modules = NULL;
    size_t shipmodule_count = 0;

    RID _dragging = GetInvalidId();
    Vector2 _dragging_mouse_offset = Vector2Zero();

    ShipModuleSlot _dragging_origin;

    int Load(const DataNode* data);
    const ShipModuleClass* GetModuleByRID(RID index) const;

    void DrawShipModule(RID index, bool inactive) const;

    void InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect);
    void DirectSwap(ShipModuleSlot slot);
    void UpdateDragging();
    bool IsDropTank(RID module, resources::T rsc) const;
    
    struct{
        RID heatshield = RID(0);
        RID droptank_water = RID(0);
        RID droptank_hydrogen = RID(0);
    } expected_modules;
};

int LoadShipModules(const DataNode* data);
const ShipModuleClass* GetModule(RID id);

#endif  // SHIP_MODULES_H