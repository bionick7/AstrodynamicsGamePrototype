#ifndef PLANET_H
#define PLANET_H
#include "basic.h"
#include "astro.h"
#include "camera.h"

#define ORBIT_BUFFER_SIZE 512

typedef struct Planet {
    char name[100];
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    bool mouse_hover;
    int id;
} Planet;


double PlanetScreenRadius(const Planet* planet);
double PlanetGetDVFromExcessVelocity(Planet* planet, Vector2 vel);
bool PlanetHasMouseHover(const Planet* ship, double* distance);
void PlanetUpdate(Planet* planet);
void PlanetDraw(Planet* planet, const DrawCamera* camera);
void PlanetDrawUI(Planet* planet, const DrawCamera* camera);

#endif  // PLANET_H
