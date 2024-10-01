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

#define PRICE_TREND_SIZE 31

typedef int64_t cost_t;

struct ResourceData {
    int resource_index = -1;
    PermaString description;

    cost_t min_cost;
    cost_t max_cost;
    cost_t default_cost;
    cost_t cost_volatility;  // Deviation of day-to-day change of error
    cost_t max_noise;        // Allowable amplitude of error
};

struct PlanetaryEconomy {
    resource_count_t resource_stock[resources::MAX];
    resource_count_t resource_capacity[resources::MAX];
    resource_count_t native_resource_delta[resources::MAX];
    resource_count_t resource_delta[resources::MAX];
    resource_count_t writable_resource_delta[resources::MAX];
    resource_count_t delivered_resources_today[resources::MAX];

    cost_t resource_price[resources::MAX];
    cost_t resource_noise[resources::MAX];

    cost_t price_history[resources::MAX*PRICE_TREND_SIZE];
    bool trading_accessible;

    PlanetaryEconomy();

    void Update(RID planet);
    void AdvanceEconomy();
    void RecalculateEconomy();
    void UIDrawResources(RID planet);
    void UIDrawEconomy(RID planet);

    resource_count_t TakeResource(resources::T resource_id, resource_count_t quantity);
    resource_count_t GiveResource(resources::T resource_id, resource_count_t quantity);
    void AddResourceDelta(resources::T resource_id, resource_count_t quantity);
    void TryPlayerTransaction(resources::T resource_id, resource_count_t quantity);
    cost_t GetPrice(resources::T resource_id, resource_count_t quantity) const;
    resource_count_t GetForPrice(resources::T resource_id, cost_t quantity) const;
};

int FindResource(const char* name, int default_);
int LoadResources(const DataNode* data);
ResourceData* GetResourceData(int resource_index);
const char* GetResourceUIRep(int resource_index);

#endif  // #ifndef PLANETARY_ECONOMY