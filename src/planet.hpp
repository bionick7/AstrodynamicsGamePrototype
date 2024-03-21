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
    bool has_atmosphere;
    Orbit orbit;
};

struct Planet {
    char name[100];
    double mu;
    double radius;
    bool has_atmosphere;
    Orbit orbit;

    int allegiance;

    int independance;
    int independance_delta;
    int base_independance_delta;  // Tied to culture,politics, ... specify per planet
    int module_independance_delta;
    StringBuilder independance_delta_log;
    int opinion;
    int opinion_delta;
    int module_opinion_delta;
    StringBuilder opinion_delta_log;

    OrbitPos position;
    PlanetaryEconomy economy;

    RID ship_module_inventory[MAX_PLANET_INVENTORY];
    IDList ship_production_queue;
    IDList module_production_queue;

    int ship_production_process;
    int module_production_process;

    bool mouse_hover;
    RID id;

    RID text3d;  // updated externally

    ShipModuleSlot current_slot;

    Planet() : Planet("UNNAMED", 0, 0) {};
    Planet(const char* name, double mu, double radius);
    void Serialize(DataNode* data) const;
    void Deserialize(Planets* planets,const DataNode* data);

    void _OnClicked();
    double ScreenRadius() const;
    double GetDVFromExcessVelocity(DVector3 vel) const;
    double GetDVFromExcessVelocity(double vel) const;
    double GetDVFromExcessVelocityPro(double vel, double parking_orbit, bool aerobreaking) const;
    Color GetColor() const;
    bool CanProduce(RID id) const;

    void Conquer(int faction, bool include_ships);

    void RecalcStats();

    ShipModuleSlot GetFreeModuleSlot() const;
    void RemoveShipModuleInInventory(int index);

    bool HasMouseHover(double* distance) const;
    void Update();
    void AdvanceShipProductionQueue();
    void AdvanceModuleProductionQueue();
    void DrawUI();
    void _UIDrawModuleProduction();
    void _UIDrawShipProduction();
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
    RID GetIdByName(const char* planet_name) const;
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
