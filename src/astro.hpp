#ifndef ASTRO_H
#define ASTRO_H

#include "basic.hpp"
#include "time.hpp"

struct OrbitPos {
    timemath::Time time;
    Vector2 cartesian;
    double M;
    double θ;
    double r;
    double longuitude;
};

struct Orbit {
    double mu;
    double sma, ecc, lop;
    timemath::Time epoch;
    bool prograde;

    Orbit();
    Orbit(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, 
          double mu, timemath::Time epoch, bool is_prograde);
    Orbit(Vector2 pos, Vector2 vel, timemath::Time t, double mu);
    Orbit(OrbitPos pos1, OrbitPos pos2, timemath::Time time_at_pos1, 
          double sma, double mu, bool is_prograde, bool cut_focus);
    
    OrbitPos GetPosition(timemath::Time time) const;
    Vector2 GetVelocity(OrbitPos pos) const;
    timemath::Time GetTimeUntilFocalAnomaly(double θ, timemath::Time start_time) const;
    double GetMeanMotion() const;
    timemath::Time GetPeriod() const;
    void Inspect() const;
    void Update(timemath::Time time, Vector2* position, Vector2* velocity) const;
    void Sample(Vector2* buffer, int buffer_size) const;
    void SampleWithOffset(Vector2* buffer, int buffer_size, double offset) const;
    void SampleBounded(double bound1, double bound2, Vector2* buffer, 
                       int buffer_size, double offset) const;
    void Draw(Color color) const;
    void DrawWithOffset(double offset, Color color) const;
    void DrawBounded(OrbitPos bound1, OrbitPos bound2, double offet, Color color) const;
};


void HohmannTransfer(
    const Orbit* from, const Orbit* to, timemath::Time t0, 
    timemath::Time* departure, timemath::Time* arrival, 
    double* dv1, double* dv2
);

#endif // ASTRO_H
