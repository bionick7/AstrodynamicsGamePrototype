#ifndef PLANETARY_ECONOMY
#define PLANETARY_ECONOMY
#include "basic.hpp"
#include "datanode.hpp"
#include "constants.hpp"

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

#define X(upper, lower) RESOURCE_##upper,
enum ResourceType {
    RESOURCE_NONE = -1,
    X_RESOURCES
    RESOURCE_MAX,
};
#undef X

#define X(upper, lower) #lower,
static const char* resource_names[RESOURCE_MAX] = {
    X_RESOURCES
};
#undef X

//#define X(upper, lower) ICON_##upper "\0"
#define X(upper, lower) ICON_##upper,
static const char* resource_icons[RESOURCE_MAX] = {
    X_RESOURCES
};
#undef X

struct ResourceTransfer {
    ResourceType resource_id;
    resource_count_t quantity;

    ResourceTransfer() {
        resource_id = ResourceType::RESOURCE_NONE; 
        quantity = 0;
    }

    ResourceTransfer(ResourceType p_resource_id, resource_count_t p_quantity) { 
        resource_id = p_resource_id; 
        quantity = p_quantity; 
    }

    ResourceTransfer Inverted() const;
};

#define RESOURCE_NAME_MAX_SIZE 30
#define RESOURCE_DESCRIPTION_MAX_SIZE 1000
#define PRICE_TREND_SIZE 31

typedef int64_t cost_t;

struct ResourceData {
    char name[RESOURCE_NAME_MAX_SIZE];
    char descrption[RESOURCE_DESCRIPTION_MAX_SIZE];

    cost_t min_cost;
    cost_t max_cost;
    cost_t default_cost;
    cost_t cost_volatility;  // Deviation of day-to-day change of error
    cost_t max_noise;        // Allowable aplitude of error
};

struct PlanetaryEconomy {
    resource_count_t resource_stock[RESOURCE_MAX];
    resource_count_t resource_capacity[RESOURCE_MAX];
    resource_count_t native_resource_delta[RESOURCE_MAX];
    resource_count_t resource_delta[RESOURCE_MAX];
    resource_count_t writable_resource_delta[RESOURCE_MAX];

    cost_t resource_price[RESOURCE_MAX];
    cost_t resource_noise[RESOURCE_MAX];

    cost_t price_history[RESOURCE_MAX*PRICE_TREND_SIZE];
    bool trading_accessible;

    PlanetaryEconomy();

    void Update();
    void AdvanceEconomy();
    void RecalcEconomy();
    void UIDrawResources(ResourceTransfer transfer, ResourceTransfer fuel);
    void UIDrawEconomy(ResourceTransfer transfer, ResourceTransfer fuel);

    ResourceTransfer DrawResource(ResourceTransfer transfer);
    ResourceTransfer GiveResource(ResourceTransfer transfer);
    void AddResourceDelta(ResourceTransfer transfer);
    void TryPlayerTransaction(ResourceTransfer transfer);
    cost_t GetPrice(ResourceType resource, resource_count_t quantity) const;
    resource_count_t GetForPrice(ResourceType resource, cost_t quantity) const;
};

int FindResource(const char* name, int default_);
int LoadResources(const DataNode* data);
ResourceData* GetResourceData(int resource_index);
const char* GetResourceUIRep(int resource_index);

#endif  // #ifndef PLANETARY_ECONOMY