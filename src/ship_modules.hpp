#ifndef SHIP_MODULES_H
#define SHIP_MODULES_H

#include "basic.hpp"
#include "datanode.hpp"
#include "id_system.hpp"

struct Ship;

#define SHIP_MODULE_WIDTH 100
#define SHIP_MODULE_HEIGHT 50

struct ShipModuleClass {
    enum {
        INVALID_MODULE,
        WATER_EXTRACTOR,
        HEAT_SHIELD
    } module_type = INVALID_MODULE;  // Placeholder for functionality


    double mass;  // kg
    char name[100];
    char description[512];

    ShipModuleClass();
    void Update(Ship* ship) const;
    const char* id;
};

struct ShipModuleSlot {
    enum ShipModuleSlotType { DRAGGING_FROM_SHIP, DRAGGING_FROM_PLANET };
    RID entity = GetInvalidId();
    int index = -1;
    ShipModuleSlotType type;

    ShipModuleSlot() = default;
    ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_type);

    void SetSlot(RID module) const;
    RID GetSlot() const;
    void AssignIfValid(ShipModuleSlot other);
    bool IsReachable(ShipModuleSlot other);
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
    void UpdateDragging();
};

int LoadShipModules(const DataNode* data);
const ShipModuleClass* GetModule(RID id);

#endif  // SHIP_MODULES_H