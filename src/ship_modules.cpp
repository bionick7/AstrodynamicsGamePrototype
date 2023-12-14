#include "ship_modules.hpp"
#include "global_state.hpp"
#include "ship.hpp"

std::map<std::string, entity_id_t> shipmodule_ids = std::map<std::string, entity_id_t>();
ShipModuleClass* ship_modules = NULL;
size_t shipmodule_count = 0;

ShipModuleClass::ShipModuleClass() {
    double mass = 0.0;
    name[0] = '\0';
    description[0] = '\0';
}

void ShipModuleClass::Update(Ship* ship) const {
    switch (module_type) {
    case WATER_EXTRACTOR:{
        if (!ship->is_parked) break;
        if (!GlobalGetState()->calendar.IsNewDay()) break;
        Planet* planet = GetPlanet(ship->parent_planet);
        planet->economy.GiveResource(ResourceTransfer(RESOURCE_WATER, 1));
        break;}
    case HEAT_SHIELD:{
        NOT_IMPLEMENTED
        break;}
    default:
    case INVALID_MODULE:
        break;
    }
}

int LoadShipModules(const DataNode* data) {
    shipmodule_count = data->GetArrayChildLen("shipmodules");
    delete[] ship_modules;
    ship_modules = new ShipModuleClass[shipmodule_count];
    for(int i=0; i < shipmodule_count; i++) {
        DataNode* module_data = data->GetArrayChild("shipmodules", i);
        ship_modules[i].mass = module_data->GetF("mass");
        strcpy(ship_modules[i].name, module_data->Get("name"));
        strcpy(ship_modules[i].description, module_data->Get("description"));

        const char* id = module_data->Get("id", "_");
        if (strcmp(id, "shpmod_water_extractor") == 0) 
            ship_modules[i].module_type = ShipModuleClass::WATER_EXTRACTOR;
        if (strcmp(id, "shpmod_heatshield") == 0) 
            ship_modules[i].module_type = ShipModuleClass::HEAT_SHIELD;

        auto pair = shipmodule_ids.insert_or_assign(id, i);
        ship_modules[i].id = pair.first->first.c_str();  // points to string in dictionary
    }
    return shipmodule_count;
}

entity_id_t GetModuleIndexById(const char* id) {
    auto find = shipmodule_ids.find(id);
    if (find == shipmodule_ids.end()) {
        return GetInvalidId();
    }
    return find->second;
}

const ShipModuleClass* GetModuleByIndex(entity_id_t index) {
    if (ship_modules == NULL) {
        ERROR("Ship modules uninitialized")
        return NULL;
    }
    if (index >= shipmodule_count) {
        ERROR("Invalid ship module index (%d >= %d or negative)", index, shipmodule_count)
        return NULL;
    }
    return &ship_modules[index];
}
