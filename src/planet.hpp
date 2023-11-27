#ifndef PLANET_H
#define PLANET_H
#include "basic.hpp"
#include "astro.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"
#include "logging.hpp"
#include "buildings.hpp"
#include "planetary_economy.hpp"

#define MAX_PLANET_BUILDINGS 20

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

    resource_count_t stats[STAT_MAX];
    BuildingInstance buildings[MAX_PLANET_BUILDINGS];

    bool mouse_hover;
    entity_id_t id;

    Planet() : Planet("UNNAMED", 0, 0) {};
    Planet(const char* name, double mu, double radius);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void _OnClicked();
    double ScreenRadius() const;
    double GetDVFromExcessVelocity(Vector2 vel) const;

    void RecalcStats();
    void RequestBuild(int slot, building_index_t building_class);

    bool HasMouseHover(double* distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf);
    void DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer, double fuel_draw);
};

const PlanetNature* GetParentNature();
int LoadEphemerides(const DataNode* data);

#endif  // PLANET_H
