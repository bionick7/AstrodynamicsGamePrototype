#ifndef ASTRO_H
#define ASTRO_H

#include "basic.hpp"
#include "time.hpp"

struct Orbit {
    double mu;
    double sma, ecc, lop;
    timemath::Time epoch;
    bool prograde;
};

struct OrbitPos {
    timemath::Time time;
    Vector2 cartesian;
    double M;
    double θ;
    double r;
    double longuitude;
};

Orbit OrbitFromElements(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, double mu, timemath::Time epoch, bool is_prograde);
Orbit OrbitFromCartesian(Vector2 pos, Vector2 vel, timemath::Time t, double mu);
Orbit OrbitFrom2PointsAndSMA(OrbitPos pos1, OrbitPos pos2, timemath::Time time_at_pos1, double sma, double mu, bool is_prograde, bool cut_focus);
OrbitPos OrbitGetPosition(const Orbit* orbit, timemath::Time time);
Vector2 OrbitGetVelocity(const Orbit* orbit, OrbitPos pos);
timemath::Time OrbitGetTimeUntilFocalAnomaly(const Orbit* orbit, double θ, timemath::Time start_time);
double OrbitGetMeanMotion(const Orbit* orbit);
timemath::Time OrbitGetPeriod(const Orbit* orbit);
void OrbitPrint(const Orbit* orbit);
void UpdateOrbit(const Orbit* orbit, timemath::Time time, Vector2* position, Vector2* velocity);

void SampleOrbit(const Orbit* orbit, Vector2* buffer, int buffer_size);
void SampleOrbitWithOffset(const Orbit* orbit, Vector2* buffer, int buffer_size, double offset);
void SampleOrbitBounded(const Orbit* orbit, double bound1, double bound2, Vector2* buffer, int buffer_size, double offset);

void DrawOrbit(const Orbit* orbit, Color color);
void DrawOrbitWithOffset(const Orbit* orbit, double offset, Color color);
void DrawOrbitBounded(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2, double offet, Color color);

void HohmannTransfer(const Orbit* from, const Orbit* to, timemath::Time t0, timemath::Time* departure, timemath::Time* arrival, double* dv1, double* dv2);

#endif // ASTRO_H
