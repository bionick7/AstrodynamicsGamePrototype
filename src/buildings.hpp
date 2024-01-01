#ifndef BUILDINGS_H
#define BUILDINGS_H
#include "basic.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "id_system.hpp"

typedef uint16_t building_index_t;
const building_index_t BUILDING_INDEX_INVALID = UINT16_MAX;
#define BUILDING_NAME_MAX_SIZE 64
#define BUILDING_DESCRIPTION_MAX_SIZE 1024

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

void BuildingConstructionOpen(RID planet, int slot_index);
void BuildingConstructionClose();
bool BuildingConstructionIsOpen();
void BuildingConstructionUI();

#endif  // BUILDINGS_H
