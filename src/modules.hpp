#ifndef MODULES_H
#define MODULES_H
#include "basic.hpp"
#include "datanode.hpp"

typedef double resource_count_t;
typedef uint16_t module_index_t;
const module_index_t MODULE_INDEX_INVALID = UINT16_MAX;

struct ResourceTransfer {
    int resource_id;
    resource_count_t quantity;
};

#define EMPTY_TRANSFER (ResourceTransfer) {-1, 0}

enum ResourceType {
    RESOURCE_NONE = -1,
    RESOURCE_WATER = 0,
    RESOURCE_FOOD,
    RESOURCE_METAL,
    RESOURCE_ELECTRONICS,
    //RESOURCE_ORGANICS,
    RESOURCE_MAX,
};

#define MODULE_NAME_MAX_SIZE 64
#define MODULE_DESCRIPTION_MAX_SIZE 1024
#define RESOURCE_NAME_MAX_SIZE 30

static const char resources_names[RESOURCE_MAX][RESOURCE_NAME_MAX_SIZE] = {
    "water", // RESOURCE_WATER
    "food",  // RESOURCE_FOOD
    "metal",  // RESOURCE_METAL
    "electronics"  // RESOURCE_ELECTRONICS
};

enum StatType {
    STAT_NONE = -1,
    STAT_POPULATION = 0,
    STAT_WORKFORCE,
    STAT_MAX,
};

static const char stat_names[STAT_MAX][RESOURCE_NAME_MAX_SIZE] = {
    "population", // STAT_POPULATION
    "workforce" // STAT_POPULATION
};

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt);

struct ModuleClass {
    char name[MODULE_NAME_MAX_SIZE];
    char description[MODULE_DESCRIPTION_MAX_SIZE];
    resource_count_t resource_delta_contributions[RESOURCE_MAX];
    resource_count_t build_costs[RESOURCE_MAX];
    resource_count_t stat_contributions[STAT_MAX];
    resource_count_t stat_required[STAT_MAX];

    const char* id = "INVALID ID - MODULE CLASS LOADING ERROR";
};

struct ModuleInstance {
    bool disabled;
    module_index_t class_index;

    ModuleInstance() : ModuleInstance(MODULE_INDEX_INVALID) {};
    ModuleInstance(module_index_t class_index);

    bool IsValid() const;
    void Effect(resource_count_t* resource_delta, resource_count_t* stats);
    bool UIDraw();
};

int LoadModules(const DataNode* datanode);
void WriteModulesToFile(const char* filepath);
module_index_t GetModuleIndexById(const char* id);
const ModuleClass* GetModuleByIndex(module_index_t index);

void ModuleConstructionOpen(entity_id_t planet, int slot_index);
void ModuleConstructionClose();
bool ModuleConstructionIsOpen();
void ModuleConstructionUI();

#endif  // MODULES_H

