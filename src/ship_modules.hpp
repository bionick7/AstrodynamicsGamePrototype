#ifndef SHIP_MODULES_H
#define SHIP_MODULES_H

#include "basic.hpp"
#include "datanode.hpp"

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
    enum DrawUIRet {
        NONE,
        CREATE,
        DELETE,
        SELECT
    };
    void Update(Ship* ship) const;
    const char* id;
};

struct ShipModuleSlot {
    enum ShipModuleSlotType { DRAGGING_FROM_SHIP, DRAGGING_FROM_PLANET };
    entity_id_t entity = GetInvalidId();
    int index = -1;
    ShipModuleSlotType type;

    ShipModuleSlot() = default;
    ShipModuleSlot(entity_id_t p_entity, int p_index, ShipModuleSlotType p_type);

    void SetSlot(entity_id_t module) const;
    entity_id_t GetSlot() const;
    void AssignIfValid(ShipModuleSlot other);
    bool IsReachable(ShipModuleSlot other);
};

struct ShipModules {
    std::map<std::string, entity_id_t> shipmodule_ids = std::map<std::string, entity_id_t>();
    ShipModuleClass* ship_modules = NULL;
    size_t shipmodule_count = 0;

    entity_id_t _dragging = GetInvalidId();
    Vector2 _dragging_mouse_offset = Vector2Zero();

    ShipModuleSlot _dragging_origin;

    int Load(const DataNode* data);
    entity_id_t GetModuleIndexById(const char* id) const;
    const ShipModuleClass* GetModuleByIndex(entity_id_t index) const;

    ShipModuleClass::DrawUIRet DrawShipModule(entity_id_t index) const;

    void InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect);
    void UpdateDragging();
};

int LoadShipModules(const DataNode* data);
const ShipModuleClass* GetModule(entity_id_t id);

#endif  // SHIP_MODULES_H