#ifndef SHIP_MODULES_H
#define SHIP_MODULES_H

#include "basic.hpp"
#include "datanode.hpp"

struct Ship;

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

int LoadShipModules(const DataNode* data);
entity_id_t GetModuleIndexById(const char* id);
const ShipModuleClass* GetModuleByIndex(entity_id_t index);

#endif  // SHIP_MODULES_H