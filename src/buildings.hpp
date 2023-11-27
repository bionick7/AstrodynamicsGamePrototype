#ifndef BUILDINGS_H
#define BUILDINGS_H
#include "basic.hpp"
#include "datanode.hpp"

typedef double resource_count_t;
typedef uint16_t building_index_t;
const building_index_t BUILDING_INDEX_INVALID = UINT16_MAX;

enum ResourceType {
    RESOURCE_NONE = -1,
    RESOURCE_WATER = 0,
    RESOURCE_FOOD,
    RESOURCE_METAL,
    RESOURCE_ELECTRONICS,
    //RESOURCE_ORGANICS,
    RESOURCE_MAX,
};

struct ResourceTransfer {
    ResourceType resource_id;
    resource_count_t quantity;
};

#define EMPTY_TRANSFER (ResourceTransfer) {RESOURCE_NONE, 0}

#define BUILDING_NAME_MAX_SIZE 64
#define BUILDING_DESCRIPTION_MAX_SIZE 1024
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
    "workforce" // STAT_WORKFORCE
};

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt);

struct BuildingClass {
    char name[BUILDING_NAME_MAX_SIZE];
    char description[BUILDING_DESCRIPTION_MAX_SIZE];
    resource_count_t resource_delta_contributions[RESOURCE_MAX];
    resource_count_t build_costs[RESOURCE_MAX];
    resource_count_t stat_contributions[STAT_MAX];
    resource_count_t stat_required[STAT_MAX];

    const char* id = "INVALID ID - BUILDING CLASS LOADING ERROR";
};

struct BuildingInstance {
    bool disabled;
    building_index_t class_index;

    BuildingInstance() : BuildingInstance(BUILDING_INDEX_INVALID) {};
    BuildingInstance(building_index_t class_index);

    bool IsValid() const;
    void Effect(resource_count_t* resource_delta, resource_count_t* stats);
    bool UIDraw();
};

int LoadBuildings(const DataNode* datanode);
void WriteBuildingsToFile(const char* filepath);
building_index_t GetBuildingIndexById(const char* id);
const BuildingClass* GetBuildingByIndex(building_index_t index);

void BuildingConstructionOpen(entity_id_t planet, int slot_index);
void BuildingConstructionClose();
bool BuildingConstructionIsOpen();
void BuildingConstructionUI();

#endif  // BUILDINGS_H

