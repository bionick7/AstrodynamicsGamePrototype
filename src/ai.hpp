#ifndef AI_H
#define AI_H

#include "id_system.hpp"
#include "planetary_economy.hpp"

struct AbstractTransfer {
    RID departure_planet;
    RID arrival_planet;
    ResourceTransfer resource_transfer;
    resource_count_t fuel;
};

struct Ship;
struct Planet;

struct AIBlackboard {
    int faction;
    RID rally_point_planet;

    double* resource_transfer_tensor = NULL;

    void Initialize();
    ~AIBlackboard();

    double CalcTransferUtility(AbstractTransfer atf) const;
    RID ShipProductionRequest() const;
    RID ModuleProductionRequest() const;

    void HighLevelFactionAI();

    void _LowLevelHandleMilitaryShip(Ship* ship) const;
    void _LowLevelHandleCivilianShip(Ship* ship) const;
    void _LowLevelHandlePlanet(Planet* planet) const;
    void LowLevelFactionAI() const;
};

#endif  // AI_H
