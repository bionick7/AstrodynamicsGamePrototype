#ifndef SHIP_MODULES_H
#define SHIP_MODULES_H

#include "basic.hpp"
#include "datanode.hpp"
#include "id_system.hpp"
#include "planetary_economy.hpp"
#include "ui.hpp"

struct Ship;

#define SHIP_MAX_MODULES 16
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
    X(BOARDING_DEFENSE, boarding_defense)


struct ShipStats {  // Better enum class, since you can treat the enum as integer
    enum {
        #define X(upper, lower) upper,
        X_SHIP_STATS
        #undef X
        MAX,
    };
};

static const char* ship_stat_names[] = {
    #define X(upper, lower) #lower,
    X_SHIP_STATS
    #undef X
};

static_assert(ShipStats::MAX == sizeof(ship_stat_names) / sizeof(ship_stat_names[0]));

struct ShipVariables {
    enum {
        KINETIC_ARMOR,
        ENERGY_ARMOR,
        CREW,

        MAX,
    };
};

static const char* ship_variable_names[] = {
    "kinetic_armor",
    "energy_armor",
    "crew",
};

static_assert(ShipVariables::MAX == sizeof(ship_variable_names) / sizeof(ship_variable_names[0]));


namespace ModuleType {
    static const char* names[] = {
        "large",
        "medium",
        "small",
        "free",
        "armor",
        "droptank",
        "any",
    };

    static Color colors[] = {
        Palette::ui_main,
        Palette::ui_alt,
        Palette::ui_dark,
        Palette::bg,
        Palette::interactable_main,
        Palette::interactable_alt,
        Palette::green,
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

    ModuleType::T FromString(const char* name);
    bool IsCompatible(ModuleType::T from, ModuleType::T to);
};

#define MODULE_CONFIG_MAX_NEIGHBOURS 4

struct ShipModuleClass {
    int delta_stats[ShipStats::MAX];
    int required_stats[ShipStats::MAX];
    resource_count_t production[RESOURCE_MAX];
    resource_count_t consumption[RESOURCE_MAX];
    resource_count_t construction_resources[RESOURCE_MAX];
    int construction_batch_size;
    uint64_t planets_restriction;

    double mass;  // kg
    char name[100];
    char description[512];
    bool has_activation_requirements;
    int construction_time;
    bool is_hidden;
    ModuleType::T type;

    AtlasPos icon_index;

    ShipModuleClass();
    void MouseHintWrite() const;
    void UpdateStats(Ship* ship) const;
    void UpdateCustom(Ship* ship) const;
    bool HasDependencies() const;
    int GetConstructionTime() const;
    const char* id;
};

struct ShipModuleSlot {
    enum ShipModuleSlotType { DRAGGING_FROM_SHIP, DRAGGING_FROM_PLANET };
    RID entity = GetInvalidId();
    int index = -1;
    ShipModuleSlotType origin_type;
    ModuleType::T module_type;

    ShipModuleSlot() = default;
    ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_origin_type, ModuleType::T p_module_type);

    void SetSlot(RID module) const;
    RID GetSlot() const;
    void AssignIfValid(ShipModuleSlot other);
    bool IsValid() const;
    bool IsReachable(ShipModuleSlot other) const;
};

struct ModuleConfiguration {
    int module_count = 0;
    ModuleType::T types[SHIP_MAX_MODULES];
    int neighbours[SHIP_MAX_MODULES*MODULE_CONFIG_MAX_NEIGHBOURS];
    Vector2 draw_offset[SHIP_MAX_MODULES];
    Rectangle draw_space;

    void Load(const DataNode* data);
    void Draw(Ship* ship) const;
    bool IsAdjacent(ShipModuleSlot lhs, ShipModuleSlot rhs) const;
};

struct ShipModules {
    std::map<std::string, RID> shipmodule_ids = std::map<std::string, RID>();
    ShipModuleClass* ship_modules = NULL;
    size_t shipmodule_count = 0;

    RID _dragging = GetInvalidId();
    Vector2 _dragging_mouse_offset = Vector2Zero();

    ShipModuleSlot _dragging_origin;

    int Load(const DataNode* data);
    RID GetModuleRIDFromStringId(const char* id) const;
    const ShipModuleClass* GetModuleByRID(RID index) const;

    void DrawShipModule(RID index) const;

    void InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect);
    void DirectSwap(ShipModuleSlot slot);
    void UpdateDragging();
    
    struct{
        RID small_yard_1;
        RID small_yard_2;
        RID small_yard_3;
        RID small_yard_4;
        RID heatshield;
        RID droptank;
    } expected_modules;
};

int LoadShipModules(const DataNode* data);
const ShipModuleClass* GetModule(RID id);

#endif  // SHIP_MODULES_H