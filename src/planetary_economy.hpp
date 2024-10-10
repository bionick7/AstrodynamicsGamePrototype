#ifndef PLANETARY_ECONOMY
#define PLANETARY_ECONOMY
#include "basic.hpp"
#include "datanode.hpp"
#include "constants.hpp"
#include "id_system.hpp"
#include "string_builder.hpp"

const double KG_PER_COUNT = 10e3;

typedef int resource_count_t;

double ResourceCountsToKG(resource_count_t counts);
resource_count_t KGToResourceCounts(double mass);

#define X_RESOURCES \
    X(WATER,       water) \
    X(HYDROGEN,    hydrogen) \
    X(OXYGEN,      oxygen) \
    X(ROCK,        rock) \
    X(IRON_ORE,    iron_ore) \
    X(STEEL,       steel) \
    X(ALUMINIUM_ORE,aluminium_ore) \
    X(ALUMINIUM,   aluminium) \
    X(FOOD,        food) \
    X(BIOMASS,     biomass) \
    X(WASTE,       waste) \
    X(CO2,         co2) \
    X(CARBON,      carbon) \
    X(POLYMERS,    polymers) \
    X(ELECTRONICS, electronics)


namespace resources {
#define X(upper, lower) upper,
    enum T {
        NONE = -1,
        X_RESOURCES
        MAX,
    };
#undef X
#define X(upper, lower) #lower,
    static const char* names[resources::MAX] = { X_RESOURCES };
#undef X
#define X(upper, lower) ICON_##upper,
    static const char* icons[resources::MAX] = { X_RESOURCES };
#undef X
}

struct ResourceData {
    int resource_index = -1;
    PermaString display_name;
    PermaString description;
};

struct PlanetaryEconomy {
    resource_count_t resource_stock[resources::MAX];
    resource_count_t resource_capacity[resources::MAX];
    resource_count_t resource_delta[resources::MAX];
    resource_count_t native_resource_delta[resources::MAX];
    resource_count_t writable_resource_delta[resources::MAX];
    resource_count_t delivered_resources_today[resources::MAX];

    PlanetaryEconomy();

    void Update(RID planet);
    void UIDrawResources(RID planet);

    resource_count_t TakeResource(resources::T resource_id, resource_count_t quantity);
    resource_count_t GiveResource(resources::T resource_id, resource_count_t quantity);
    void AddResourceDelta(resources::T resource_id, resource_count_t quantity);
};

int FindResource(const char* name, int default_);
int LoadResources(const DataNode* data);
ResourceData* GetResourceData(int resource_index);
const char* GetResourceUIRep(int resource_index);

#endif  // #ifndef PLANETARY_ECONOMY