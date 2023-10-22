#ifndef ASTRO_H
#define ASTRO_H

#include "basic.h"

typedef struct Orbit {
    double mu;
    double sma, ecc, lop;
    time_type period;
    bool prograde;
} Orbit;

typedef struct OrbitPos {
    time_type time;
    Vector2 cartesian;
    double M;
    double θ;
    double r;
    double longuitude;
} OrbitPos;


Orbit OrbitFromElements(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, double mu, time_type period, bool is_prograde);
Orbit OrbitFromCartesian(Vector2 pos, Vector2 vel, time_type t, double mu);
Orbit OrbitFrom2PointsAndSMA(OrbitPos pos1, OrbitPos pos2, time_type time_at_pos1, double sma, double mu, bool is_prograde, bool cut_focus);
OrbitPos OrbitGetPosition(const Orbit* orbit, time_type time);
Vector2 OrbitGetVelocity(const Orbit* orbit, OrbitPos pos);
time_type OrbitGetTimeUntilFocalAnomaly(const Orbit* orbit, double θ, time_type start_time);
double OrbitGetMeanMotion(const Orbit* orbit);
time_type OrbitGetPeriod(const Orbit* orbit);
void OrbitPrint(const Orbit* orbit);
void UpdateOrbit(const Orbit* orbit, time_type time, Vector2* position, Vector2* velocity);

void SampleOrbit(const Orbit* orbit, Vector2* buffer, int buffer_size);
void SampleOrbitWithOffset(const Orbit* orbit, Vector2* buffer, int buffer_size, double offset);
void SampleOrbitBounded(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2, Vector2* buffer, int buffer_size);

void DrawOrbit(const Orbit* orbit, Color color);
void DrawOrbitWithOffset(const Orbit* orbit, double offset, Color color);
void DrawOrbitBounded(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2, Color color);

#endif // ASTRO_H
