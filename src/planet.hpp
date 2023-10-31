#ifndef PLANET_H
#define PLANET_H
#include "basic.hpp"
#include "astro.hpp"
#include "coordinate_transform.hpp"

#define RESOURCE_CAP INT
typedef double resource_count_t;

static const char resources_names[2][30] = {
    "WATER",
    "FOOD "
};

struct ResourceTransfer {
    int resource_id;
    resource_count_t quantity;
};

#define EMPTY_TRANSFER (ResourceTransfer) {-1, 0}

enum ResourceType {
    RESOURCE_NONE = -1,
    RESOURCE_WATER = 0,
    RESOURCE_FOOD,
    //RESOURCE_METALS,
    //RESOURCE_ELECTRONICS,
    //RESOURCE_ORGANICS,
    //RESOURCE_PEOPLE,
    RESOURCE_MAX,
};

ResourceTransfer ResourceTransferInvert(ResourceTransfer rt);

struct Planet {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    resource_count_t resource_stock[RESOURCE_MAX];
    resource_count_t resource_capacity[RESOURCE_MAX];
    resource_count_t resource_delta[RESOURCE_MAX];

    bool mouse_hover;
    entity_id_t id;

    void Make(const char* name, double mu, double radius);

    void _OnClicked();
    double ScreenRadius() const;
    double GetDVFromExcessVelocity(Vector2 vel) const;

    resource_count_t DrawResource(int resource, resource_count_t quantity);
    resource_count_t GiveResource(int resource, resource_count_t quantity);

    bool HasMouseHover(double* distance) const;
    void Update();
    void Draw(const CoordinateTransform* c_transf);
    void DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer);
};


#endif  // PLANET_H
