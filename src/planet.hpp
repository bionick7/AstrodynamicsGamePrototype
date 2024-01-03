#ifndef PLANET_H
#define PLANET_H
#include "astro.hpp"
#include "basic.hpp"
#include "coordinate_transform.hpp"
#include "datanode.hpp"
#include "planetary_economy.hpp"
#include "ship_modules.hpp"

#define MAX_PLANET_BUILDINGS 20
#define MAX_PLANET_INVENTORY 40

struct Planets;

struct PlanetNature {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
};

struct Planet {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    PlanetaryEconomy economy;

    resource_count_t planet_stats[static_cast<int>(PlanetStats::MAX)];
    RID ship_module_inventory[MAX_PLANET_INVENTORY];

    bool mouse_hover;
    RID id;

    ShipModuleSlot current_slot;

    Planet() : Planet("UNNAMED", 0, 0) {};
    Planet(const char* name, double mu, double radius);
    void Serialize(DataNode* data) const;
    void Deserialize(Planets* planets,const DataNode* data);

    void _OnClicked();
    double ScreenRadius() const;
    double GetDVFromExcessVelocity(Vector2 vel) const;

    void RecalcStats();

    bool AddShipModuleToInventory(RID module);
    void RemoveShipModuleInInventory(int index);

    bool HasMouseHover(double* distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf);
    void DrawUI();
    void _UIDrawInventory();
};

struct Planets {
    PlanetNature* ephemerides;
    PlanetNature parent;
    Planet* planet_array;
    int planet_count;
    int planet_array_iter;

    Planets();
    ~Planets();
    RID AddPlanet(const DataNode* data);
    RID GetIndexByName(const char* planet_name) const;
    Planet* GetPlanet(RID id) const;
    const PlanetNature* GetPlanetNature(RID id) const;
    int GetPlanetCount() const;

    const PlanetNature* GetParentNature() const;
    int LoadEphemerides(const DataNode* data);
};


Planet* GetPlanet(RID id);
Planet* GetPlanetByIndex(int index);
int LoadEphemerides(const DataNode* data);


#endif  // PLANET_H
