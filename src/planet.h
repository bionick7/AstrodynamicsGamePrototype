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
    Vector2 position;
    Vector2 velocity;
    Vector2 orbit_draw_buffer[ORBIT_BUFFER_SIZE];
} Planet;


void PlanetDraw(Planet* planet, const DrawCamera* camera);
void PlanetDrawUI(Planet* planet, const DrawCamera* camera);

#endif  // PLANET_H