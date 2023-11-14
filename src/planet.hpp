#ifndef PLANET_H
#define PLANET_H
#include "basic.hpp"
#include "astro.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"
#include "logging.hpp"
#include "modules.hpp"

#define MAX_PLANET_MODULES 32

struct Planet {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    resource_count_t resource_stock[RESOURCE_MAX];
    resource_count_t resource_capacity[RESOURCE_MAX];
    resource_count_t resource_delta[RESOURCE_MAX];

    resource_count_t stats[STAT_MAX];
    module_index_t modules[MAX_PLANET_MODULES];
    int module_count = 0;

    bool mouse_hover;
    entity_id_t id;

    Planet() { Planet("UNNAMED", 0, 0); }
    Planet(const char* name, double mu, double radius);
    void Load(const DataNode* data, double parent_mu);
    void Save(DataNode* data) const;

    void _OnClicked();
    double ScreenRadius() const;
    double GetDVFromExcessVelocity(Vector2 vel) const;

    resource_count_t DrawResource(int resource, resource_count_t quantity);
    resource_count_t GiveResource(int resource, resource_count_t quantity);

    void RecalcStats();

    bool HasMouseHover(double* distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf);
    void DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer);
};


#endif  // PLANET_H
