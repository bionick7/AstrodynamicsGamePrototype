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
    //RESOURCE_METALS,
    //RESOURCE_ELECTRONICS,
    //RESOURCE_ORGANICS,
    //RESOURCE_PEOPLE,
    RESOURCE_MAX,
};

static const char resources_names[RESOURCE_MAX][30] = {
    "water", // RESOURCE_WATER
    "food"  // RESOURCE_FOOD
};

enum StatType {
    STAT_NONE = -1,
    STAT_POPULATION = 0,
    STAT_MAX,
};

static const char stat_names[STAT_MAX][30] = {
    "population", // STAT_POPULATION
};


ResourceTransfer ResourceTransferInvert(ResourceTransfer rt);

#define MODULE_NAME_MAX_SIZE 128
#define MODULE_DESCRIPTION_MAX_SIZE 2048

struct Module {
    char name[MODULE_NAME_MAX_SIZE];
    char description[MODULE_DESCRIPTION_MAX_SIZE];

    void Effect(resource_count_t* resource_delta, resource_count_t* stats) const;
    resource_count_t resource_delta_contributions[RESOURCE_MAX];
    resource_count_t stat_contributions[STAT_MAX];
};

int LoadModules(const DataNode* datanode);
module_index_t GetModuleIndexById(const char* id);
Module* GetModuleByIndex(module_index_t index);

#endif  // MODULES_H

