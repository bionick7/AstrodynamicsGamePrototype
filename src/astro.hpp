#ifndef ASTRO_H
#define ASTRO_H

#include "basic.hpp"
#include "time.hpp"
#include "string_builder.hpp"
#include "dvector3.hpp"

struct OrbitPos {
    timemath::Time time;
    DVector3 cartesian;
    double M;
    double θ;
    double r;
};

struct Orbit {
    double mu;
    double sma;
    double ecc;
    DVector3 periapsis_dir;
    DVector3 normal;
    timemath::Time epoch;

    Orbit();
    Orbit(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, 
          double mu, timemath::Time epoch, bool is_prograde);  // 2D
    Orbit(double semi_major_axis, double eccenetricity, double inclination, 
          double longuitude_of_periapsis, double right_ascention_of_ascending_node, 
          double mu, timemath::Time epoch);  // 3D
    //Orbit(DVector3 pos, DVector3 vel, timemath::Time t, double mu);
    Orbit(OrbitPos pos1, OrbitPos pos2, timemath::Time time_at_pos1, 
          double sma, double mu, int lcase);
    
    void Inspect() const;

    double GetMeanMotion() const;
    timemath::Time GetPeriod() const;

    OrbitPos GetPosition(timemath::Time time) const;
    DVector3 GetVelocity(OrbitPos pos) const;
    void Update(timemath::Time time, DVector3* position, DVector3* velocity) const;

    OrbitPos FromFocal(double focal) const;
    OrbitPos FromRightAscension(double ra) const;
    DVector3 RadialToGlobal(DVector3 inp, DVector3 z) const;
    timemath::Time GetTimeUntilFocalAnomaly(double θ, timemath::Time start_time) const;

    Vector2 GetMousPosOnPlane() const;

    /*void Sample(DVector3* buffer, int buffer_size) const;
    void SampleWithOffset(DVector3* buffer, int buffer_size, double offset) const;*/
    void SampleBounded(double bound1, double bound2, DVector3* buffer, 
                       int buffer_size, double offset) const;
    void Draw(Color color) const;
    void DrawWithOffset(double offset, Color color) const;
};

struct OrbitSegment {
    const Orbit* orbit; // not owning
    OrbitPos bound1, bound2;
    bool is_full_circle;

    OrbitSegment(const Orbit* orbit);
    OrbitSegment(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2);

    Vector2 ClosestApprox(Vector2 p, bool letExtrude) const;
    Vector2 AdjustmentToClosest(Vector2 p) const;
    OrbitPos TraceRay(Vector3 origin, Vector3 ray, float* distance, 
                      timemath::Time* mouseover_time, Vector3* local_mouseover_pos) const;
};

void HohmannTransfer(
    const Orbit* from, const Orbit* to, timemath::Time t0, 
    timemath::Time* departure, timemath::Time* arrival, 
    double* dv1, double* dv2
);
void GetDVTable(StringBuilder* sb, bool include_arobreaks);

#endif // ASTRO_H
