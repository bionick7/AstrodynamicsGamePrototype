#ifndef PLANET_H
#define PLANET_H
#include "basic.h"
#include "astro.h"
#include "camera.h"

#define ORBIT_BUFFER_SIZE 512

typedef struct Planet {
    const char* name;
    double mu;
    double radius;
    Orbit orbit;
    OrbitPos position;

    int id;
} Planet;

double PlanetGetDVFromExcessVelocity(Planet* planet, Vector2 vel);
void PlanetUpdate(Planet* planet);
void PlanetDraw(Planet* planet, const DrawCamera* camera);
void PlanetDrawUI(Planet* planet, const DrawCamera* camera);

#endif  // PLANET_H
