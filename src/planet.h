#ifndef PLANET_H
#define PLANET_H
#include "basic.h"
#include "astro.h"
#include "camera.h"

#define RESOURCE_CAP INT
typedef double resource_count_t;

static const char resources_names[2][30] = {
    "WATER",
    "FOOD "
};

ENUM_DECL(ResourceType) {
    RESOURCE_NONE = -1,
    RESOURCE_WATER = 0,
    RESOURCE_FOOD,
    //RESOURCE_METALS,
    //RESOURCE_ELECTRONICS,
    //RESOURCE_ORGANICS,
    //RESOURCE_PEOPLE,
    RESOURCE_MAX,
};

STRUCT_DECL(Planet) {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    resource_count_t resource_stock[RESOURCE_MAX];
    resource_count_t resource_capacity[RESOURCE_MAX];
    resource_count_t resource_delta[RESOURCE_MAX];

    bool mouse_hover;
    int id;
};

void PlanetMake(Planet* planet, const char* name, double mu, double radius);

double PlanetScreenRadius(const Planet* planet);
double PlanetGetDVFromExcessVelocity(const Planet* planet, Vector2 vel);

resource_count_t PlanetDrawResource(Planet* planet, int resource, resource_count_t quantity);
resource_count_t PlanetGiveResource(Planet* planet, int resource, resource_count_t quantity);

bool PlanetHasMouseHover(const Planet* ship, double* distance);
void PlanetUpdate(Planet* planet);
void PlanetDraw(Planet* planet, const DrawCamera* camera);
void PlanetDrawUI(Planet* planet, const DrawCamera* cam, bool upper_quadrant);

#endif  // PLANET_H
