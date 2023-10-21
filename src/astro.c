#include "astro.h"
#include "debug_drawing.h"

Vector2 _FromPolar(double radius, double phase) {
    return (Vector2) {
        radius * cos(phase),
        radius * sin(phase)
    };
}

double _True2Ecc(double θ, double e) {
    return atan(sqrt((1 - e) / (1 + e)) * tan(θ/2)) * 2;
}

double _Ecc2True(double E, double e) {
    return atan(sqrt((1 + e) / (1 - e)) * tan(E/2)) * 2;
}

double _Ecc2Mean(double E, double e) {
    return E - e * sin(E);
}

double _Mean2Ecc(double M, double e) {
    double E = M;
    double δ = M + e*sin(E) - E;
    int counter = 0;
    while (fabs(δ) > 1e-6){
        E += δ;
        δ = M + e*sin(E) - E;
        if (counter++ > 1000) {
            printf("Counter exceeded for M = %f, e = %f\n", M, e);
            return E;
        }
    }
    //printf("E = %f\n", E);
    return E;
}

double _True2Mean(double θ, double e) {
    return _Ecc2Mean(_True2Ecc(θ, e), e);
}

double _Mean2True(double M, double e) {
    return _Ecc2True(_Mean2Ecc(M, e), e);
}

double _MeanMotion(const Orbit* orbit) {
    return sqrt(orbit->mu / (orbit->sma*orbit->sma*orbit->sma)) * (orbit->prograde ? 1.0 : -1.0);
}

Orbit OrbitFromElements(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, double mu, time_type period, bool is_prograde) {
    return (Orbit) {mu, semi_major_axis, eccenetricity, longuitude_of_periapsis, period, is_prograde};
}

Orbit OrbitFromCartesian(Vector2 pos, Vector2 vel, time_type t, double mu) {
    double r = Vector2Length(pos);
    double energy = Vector2LengthSqr(vel) / 2 - mu / r;
    double ang_mom = fabs(pos.x*vel.y - pos.y*vel.x);
    Vector2 ecc_vector = Vector2Subtract(
        Vector2Scale(pos, Vector2LengthSqr(vel) / mu - 1.0 / r),
        Vector2Scale(vel, Vector2DotProduct(pos, vel) / mu)
    );
    double e = Vector2Length(ecc_vector);
    double a = -mu / (2 * energy);
    double lop = atan2(ecc_vector.y, ecc_vector.x) + ecc_vector.x > 0 ? PI : 0;
    double angular_pos = (atan2(pos.y, pos.x) + pos.x > 0 ? PI : 0);
    double mean_motion = sqrt(mu / (a*a*a)) * (ang_mom > 0.0 ? 1.0 : -1.0);
    double period = t + (angular_pos - lop) / mean_motion;
    return OrbitFromElements(a, e, lop, mu, period, ang_mom > 0);
}

Vector2 Apply2DTransform(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp) {
    return Vector2Add(origin,
        Vector2Add(
            Vector2Scale(x, inp.x),
            Vector2Scale(y, inp.y)
        )
    );
}

double fsign(double in) {
    //return ((unsigned long)((void*) &in) << 63 == 1) ? 1 : -1;
    return in > 0 ? 1 : -1;
}

Orbit OrbitFrom2PointsAndSMA(OrbitPos pos1, OrbitPos pos2, time_type time_at_pos1, double sma, double mu, bool is_prograde, bool cut_focus) {
    // goes from pos1 to pos2
    //if (!is_prograde) swap(&pos1, &pos2);
    double r1_r2_outer_prod = pos1.cartesian.x * pos2.cartesian.y - pos1.cartesian.y * pos2.cartesian.x;
    double d = Vector2Distance(pos1.cartesian, pos2.cartesian) / 2.0;
    double A = (pos2.r - pos1.r) / 2;
    double E = d / A;
    double B_sqr = d*d - A*A;
    double x_0 = - (pos2.r + pos1.r) / (2*E);
    double y_0 = fsign(r1_r2_outer_prod) * sqrt(B_sqr * (x_0*x_0 / (A*A) - 1));
    double x_f = x_0 + 2*sma / E;
    double y_f = sqrt(B_sqr * (x_f*x_f / (A*A) - 1));
    if (cut_focus) y_f = -y_f;
    double e = hypot(x_0 - x_f, y_0 - y_f) / (2 * sma);
    double p = sma * (1 - e*e);
    double θ_1 = acos((p / pos1.r - 1) / e);

    Vector2 canon_x = Vector2Normalize(Vector2Scale(Vector2Subtract(pos2.cartesian, pos1.cartesian), .5));
    Vector2 canon_y = Vector2Rotate(canon_x, PI/2);
    Vector2 periapsis_dir = Apply2DTransform((Vector2){0}, canon_x, canon_y, (Vector2) {x_0 - x_f, y_0 - y_f});

    double lop = atan2(periapsis_dir.y, periapsis_dir.x);
    θ_1 = pos1.longuitude - lop;

    //if (fabs(pos2.r - p / (1 + e*cos(pos2.longuitude + lop))) > 1e3) {
    //    θ_1 = -θ_1;
    //}
    // DEBUG DRAWING
    
    //SHOW_F(sma) SHOW_F(d) SHOW_F(A) SHOW_F(E) SHOW_F(B_sqr) SHOW_F(x_0) SHOW_F(x_f) SHOW_F(y_0) SHOW_F(y_f) SHOW_F(e) SHOW_F(p)
    //Vector2 canon_center = Vector2Scale(Vector2Add(pos1.cartesian, pos2.cartesian), .5);
    //DebugDrawLine(pos1.cartesian, pos2.cartesian);
    //DebugDrawConic(pos1.cartesian, Vector2Scale(Vector2Normalize(Vector2Subtract(pos2.cartesian, pos1.cartesian)), fabs(E)), -fabs(A));
    //DebugDrawLine(
    //    Apply2DTransform(canon_center, canon_x, canon_y, (Vector2) {x_0, y_0}),
    //    Apply2DTransform(canon_center, canon_x, canon_y, (Vector2) {x_f, y_f})
    //);
    
    double M_1 = _True2Mean(θ_1, e);
    DEBUG_SHOW_I(is_prograde)
    DEBUG_SHOW_I(cut_focus)
    DEBUG_SHOW_F(M_1)
    DEBUG_SHOW_F(θ_1)
    // TODO: what happens if the orbit is retrograde

    double period = time_at_pos1 - M_1 * sqrt(sma*sma*sma / mu) * (is_prograde ? 1.0 : -1.0);
    //period = fmod(period, sqrt(sma*sma*sma / mu) * 2*PI);
    return OrbitFromElements(sma, e, lop, mu, period, is_prograde);
}

OrbitPos OrbitGetPosition(const Orbit* orbit, time_type time) {
    OrbitPos res = {0};
    res.M = (time - orbit->period) * _MeanMotion(orbit);
    res.M = fmod(res.M, PI*2);
    res.θ = _Mean2True(res.M, orbit->ecc);
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    res.r = p / (1 + orbit->ecc * cos(res.θ));
    // printf("M = %f ; E = %f ; θ = %f r = %f\n", M, E, θ, r);
    // printf("pos.x %f pos.y 5f\n", postion->x, postion->y);
    res.longuitude = res.θ + orbit->lop;
    res.cartesian = _FromPolar(res.r, res.longuitude);
    return res;
}

Vector2 OrbitGetVelocity(const Orbit* orbit, OrbitPos pos) {
    double cot_ɣ = -orbit->ecc * sin(pos.θ) / (1 + orbit->ecc * cos(pos.θ));
    Vector2 local_vel = Vector2Scale(Vector2Normalize((Vector2) {cot_ɣ, 1}), sqrt((2*orbit->mu) / pos.r - orbit->mu/orbit->sma));
    return Vector2Rotate(local_vel, orbit->lop);
}


time_type OrbitGetTimeUntilFocalAnomaly(const Orbit* orbit, double θ, time_type start_time) {
    double mean_motion_inv = sqrt(orbit->sma*orbit->sma*orbit->sma / orbit->mu);  // s
    double period = 2 * PI * mean_motion_inv;
    double M = _True2Mean(θ, orbit->ecc);
    double reference_time = orbit->period + M *mean_motion_inv;
    time_type diff = fmod(reference_time - start_time, period);
    if (diff < 0) diff += period;
    return start_time + diff;
}

void OrbitPrint(const Orbit* orbit) {
    printf("%s a = %f m, e = %f, lop = %f", (orbit->prograde ? "" : "R"), orbit->sma, orbit->ecc, orbit->lop);
}

void UpdateOrbit(const Orbit* orbit, time_type time, Vector2* position, Vector2* velocity) {
    OrbitPos orbit_position = OrbitGetPosition(orbit, time);

    *position = orbit_position.cartesian;
    *velocity = OrbitGetVelocity(orbit, orbit_position);
}

void SampleOrbit(const Orbit* orbit, Vector2* buffer, int buffer_size) {
    /*Vector2 center = _FromPolar(orbit->sma * orbit->ecc, orbit->lop + PI);
    double semi_minor_axis = orbit->sma * sqrt(1 - orbit->ecc*orbit->ecc);
    for (int i=0; i < buffer_size - 1; i++) {
        double interpolator = (double) i / (double) buffer_size * PI * 2.0;
        buffer[i] = Vector2Add(center, 
            Vector2Rotate((Vector2) {
                orbit->sma * cos(interpolator),
                semi_minor_axis * sin(interpolator)
            }, orbit->lop)
        );
    }*/
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = (double) i / (double) buffer_size * PI * 2.0;
        double r = p / (1 + orbit->ecc*cos(θ));
        buffer[i] = _FromPolar(r, θ + orbit->lop);
    }

    buffer[buffer_size - 1] = buffer[0];
}

void SampleOrbitBounded(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2, Vector2* buffer, int buffer_size) {
    if (orbit->prograde && bound2.θ < bound1.θ) bound2.θ += PI * 2.0;
    if (!orbit->prograde && bound2.θ > bound1.θ) bound2.θ -= PI * 2.0;
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = Lerp(bound1.θ, bound2.θ, (double) i / (double) (buffer_size - 1));
        double r = p / (1 + orbit->ecc*cos(θ));
        buffer[i] = _FromPolar(r, θ + orbit->lop);
    }
}
