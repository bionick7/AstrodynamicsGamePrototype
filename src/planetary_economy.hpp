#ifndef PLANETARY_ECONOMY
#define PLANETARY_ECONOMY
#include "basic.hpp"
#include "datanode.hpp"

typedef double resource_count_t;

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

    ResourceTransfer() {
        resource_id = ResourceType::RESOURCE_NONE; 
        quantity = 0;
    }

    ResourceTransfer(ResourceType p_resource_id, resource_count_t p_quantity) { 
        resource_id = p_resource_id; 
        quantity = p_quantity; 
    }

    ResourceTransfer Inverted();
};

#define EMPTY_TRANSFER (ResourceTransfer) {RESOURCE_NONE, 0}

#define RESOURCE_NAME_MAX_SIZE 30
#define RESOURCE_DESCRIPTION_MAX_SIZE 1000
#define PRICE_TREND_SIZE 31

typedef double cost_t;

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

struct ResourceData {
    char name[RESOURCE_NAME_MAX_SIZE];
    char descrption[RESOURCE_DESCRIPTION_MAX_SIZE];

    cost_t min_cost;
    cost_t max_cost;
    cost_t default_cost;
    cost_t cost_volatility;  // Deviation of day-to-day change of error
    cost_t max_noise;        // Allowable aplitude of error
};

static const char resource_names[RESOURCE_MAX][RESOURCE_NAME_MAX_SIZE] = {
    "water", // RESOURCE_WATER
    "food",  // RESOURCE_FOOD
    "metal",  // RESOURCE_METAL
    "electronics"  // RESOURCE_ELECTRONICS
};


struct PlanetaryEconomy {
    resource_count_t resource_stock[RESOURCE_MAX];
    resource_count_t resource_capacity[RESOURCE_MAX];
    resource_count_t resource_delta[RESOURCE_MAX];

    cost_t resource_price[RESOURCE_MAX];
    cost_t resource_noise[RESOURCE_MAX];

    cost_t price_history[RESOURCE_MAX*PRICE_TREND_SIZE];
    bool trading_acessible;

    PlanetaryEconomy();

    void Update();
    void AdvanceEconomy();
    void RecalcEconomy();
    void UIDrawResources(const ResourceTransfer& transfer, double fuel_draw);
    void UIDrawEconomy(const ResourceTransfer& transfer, double fuel_draw);

    ResourceTransfer DrawResource(ResourceTransfer transfer);
    ResourceTransfer GiveResource(ResourceTransfer transfer);
    void TryPlayerTransaction(ResourceTransfer tf);
    cost_t GetPrice(ResourceType resource, resource_count_t quantity) const;
    resource_count_t GetForPrice(ResourceType resource, cost_t quantity) const;
};

int LoadResources(const DataNode* data);
ResourceData& GetResourceData(int resource_index);

#endif  // #ifndef PLANETARY_ECONOMY