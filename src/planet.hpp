#ifndef PLANET_H
#define PLANET_H
#include "astro.hpp"
#include "basic.hpp"
#include "coordinate_transform.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "ship_modules.hpp"

#define MAX_PLANET_INVENTORY 40

struct Planets;

struct PlanetNature {
    PermaString name;
    PermaString description;
    double mu;
    double radius;
    double rotation_period;
    bool has_atmosphere;
    Orbit orbit;
};

struct Planet {
    PermaString name;
    PermaString description;
    double mu;
    double radius;
    double rotation_period;
    bool has_atmosphere;
    Orbit orbit;

    int allegiance;

    int independence;
    int independence_delta;
    int base_independence_delta;  // Tied to culture,politics, ... specify per planet
    int module_independence_delta;
    StringBuilder independence_delta_log;
    int opinion;
    int opinion_delta;
    int module_opinion_delta;
    StringBuilder opinion_delta_log;

    OrbitPos position;
    PlanetaryEconomy economy;

    RID inventory[MAX_PLANET_INVENTORY];

    IDList cached_ship_list;  // Cached for quicka access (renewed every frame)

    bool mouse_hover;
    RID id;

    RID text3d;  // updated externally
    RID orbit_render_conic;
    RID sphere;

    ShipModuleSlot current_slot;

    Planet();
    void Serialize(DataNode* data) const;
    void Deserialize(Planets* planets,const DataNode* data);

    void _OnClicked();
    double ScreenRadius() const;
    double GetSOI() const;
    double GetDVFromExcessVelocity(DVector3 vel) const;
    double GetDVFromExcessVelocity(double vel) const;
    double GetDVFromExcessVelocityPro(double vel, double parking_orbit, bool aerobreaking) const;
    Color GetColor() const;
    bool CanProduce(RID id, bool check_resources, bool check_stats) const;

    void Conquer(int faction, bool include_ships);

    void RecalculateStats();

    ShipModuleSlot GetFreeModuleSlot(module_types::T type) const;
    void RemoveShipModuleInInventory(int index);
    //ProductionOrder MakeProductionOrder(RID id) const;

    double GetMousePixelDistance() const;
    void GetRandomOrbit(int index, Orbit* orbit) const;
    void Update();
    //void AdvanceShipProductionQueue();
    //void AdvanceModuleProductionQueue();
    void Draw3D() const;
    void DrawUI();
    void _UIDrawInventory();
    void _UIDrawDescription();
    void _UIDrawModuleProduction();
    void _UIDrawShipProduction();
};

struct Planets {
    PlanetNature* ephemeris;
    PlanetNature parent;
    Planet* planet_array;
    int planet_count;
    int planet_array_iter;

    Planets();
    ~Planets();
    RID AddPlanet(const DataNode* data);
    RID GetIdByName(const char* planet_name) const;
    Planet* GetPlanet(RID id) const;
    const PlanetNature* GetPlanetNature(RID id) const;
    int GetPlanetCount() const;

    const PlanetNature* GetParentNature() const;
    int LoadEphemeris(const DataNode* data);
    void Clear();
};

Planet* GetPlanet(RID id);
Planet* GetPlanetByIndex(int index);
double PlanetsMinDV(RID from, RID to, bool aerobrake);
int LoadEphemeris(const DataNode* data);

void SetPlanetTabIndex(int index);

#endif  // PLANET_H
