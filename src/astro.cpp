#include "astro.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "coordinate_transform.hpp"

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
    int counter = 0;
    double δ;
    do {
        if (e < 0.1) {
            δ = M + e*sin(E) - E;
        } else {
            δ = -(E - e*sin(E) - M) / (1 - e*cos(E));
        }
        E += δ;
        if (counter++ > 1000) {
            printf("Counter exceeded for M = %f, e = %f\n", M, e);
            return E;
        }
    } while (fabs(δ) > 1e-6);

    //printf("E = %f\n", E);
    return E;
}

double _True2Mean(double θ, double e) {
    return _Ecc2Mean(_True2Ecc(θ, e), e);
}

double _Mean2True(double M, double e) {
    return _Ecc2True(_Mean2Ecc(M, e), e);
}

double _True2EccHyp(double θ, double e) {
    return atanh(sqrt((e - 1) / (e + 1)) * tan(θ/2)) * 2;
}

double _Ecc2TrueHyp(double F, double e) {
    return atan(sqrt((e + 1) / (e - 1)) * tanh(F/2)) * 2;
}

double _Ecc2MeanHyp(double F, double e) {
    return e * sinh(F) - F;
}

double _Mean2EccHyp(double M, double e) {
    double F;
    if (M > 6 * e) {
        F = log(2*M/e);
    } else if (M < -6 * e) {
        F = -log(2*-M/e);
    } else {
        double x = sqrt(8*(e-1)/e);
        double y = asinh(3*M/(x*(e-1))) / 3;
        F = x*sinh(y);
    }
    int counter = 0;
    double δ;
    do {
        δ = (e*sinh(F) - F - M) / (e*cosh(F) - 1);
        F -= δ;
        if (counter++ > 1000) {
            printf("Counter exceeded for M = %f, e = %f\n", M, e);
            return F;
        }
    } while (fabs(δ) > 1e-6);
    //printf("E = %f\n", E);
    return F;
}

double _True2MeanHyp(double θ, double e) {
    return _Ecc2MeanHyp(_True2EccHyp(θ, e), e);
}

double _Mean2TrueHyp(double M, double e) {
    return _Ecc2TrueHyp(_Mean2EccHyp(M, e), e);
}

Orbit OrbitFromElements(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, double mu, time_type period, bool is_prograde) {
    Orbit res = {0};
    res.mu = mu;
    res.sma = semi_major_axis;
    res.ecc = eccenetricity;
    res.lop = longuitude_of_periapsis;
    res.period = period;
    res.prograde = is_prograde;
    return res;
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

Orbit OrbitFrom2PointsAndSMA(OrbitPos pos1, OrbitPos pos2, time_type time_at_pos1, double sma, double mu, bool is_prograde, bool cut_focus) {
    // https://en.wikipedia.org/wiki/Lambert%27s_problem
    // goes from pos1 to pos2
    //if (!is_prograde) Swap(&pos1, &pos2);
    double r1_r2_outer_prod = Determinant(pos1.cartesian, pos2.cartesian);
    double d = Vector2Distance(pos1.cartesian, pos2.cartesian) / 2.0;
    double A = (pos2.r - pos1.r) / 2;
    double E = d / A;
    double B_sqr = d*d - A*A;
    double x_0 = - (pos2.r + pos1.r) / (2*E);
    double y_0 = Sign(r1_r2_outer_prod) * sqrt(B_sqr * (x_0*x_0 / (A*A) - 1));
    double x_f = x_0 + 2*sma / E;
    double y_f = sqrt(B_sqr * (x_f*x_f / (A*A) - 1));
    if (cut_focus) y_f = -y_f;  // For a hyperbola this means, it's the more direct way
    double e = hypot(x_0 - x_f, y_0 - y_f) / (2 * fabs(sma));
    if (isnan(e)) {
        printf("nan\n");
    }

    Vector2 canon_x = Vector2Normalize(Vector2Scale(Vector2Subtract(pos2.cartesian, pos1.cartesian), .5));
    Vector2 canon_y = Vector2Rotate(canon_x, PI/2);
    Vector2 periapsis_dir = Apply2DTransform((Vector2){0}, canon_x, canon_y, (Vector2) {x_0 - x_f, y_0 - y_f});
    periapsis_dir = Vector2Scale(periapsis_dir, Sign(sma));

    // DEBUG DRAWING
    
    //SHOW_F(sma) SHOW_F(d) SHOW_F(A) SHOW_F(E) SHOW_F(B_sqr) SHOW_F(x_0) SHOW_F(x_f) SHOW_F(y_0) SHOW_F(y_f) SHOW_F(e) SHOW_F(p)
    // Vector2 canon_center = Vector2Scale(Vector2Add(pos1.cartesian, pos2.cartesian), .5);
    // DebugDrawLine(pos1.cartesian, pos2.cartesian);
    // DebugDrawConic(pos1.cartesian, Vector2Scale(Vector2Normalize(Vector2Subtract(pos2.cartesian, pos1.cartesian)), fabs(E)), -fabs(A));
    // DebugDrawLine(
    //     Apply2DTransform(canon_center, canon_x, canon_y, (Vector2) {x_0, y_0}),
    //     Apply2DTransform(canon_center, canon_x, canon_y, (Vector2) {x_f, y_f})
    // );

    double lop = atan2(periapsis_dir.y, periapsis_dir.x);
    double θ_1 = pos1.longuitude - lop;
    double M_1 = sma < 0 ? _True2MeanHyp(θ_1, e) : _True2Mean(θ_1, e);
    // TODO: what happens if the orbit is retrograde

    double period = time_at_pos1 - M_1 * sqrt(fabs(sma)*sma*sma / mu) * (is_prograde ? 1.0 : -1.0);
    //period = fmod(period, sqrt(sma*sma*sma / mu) * 2*PI);
    return OrbitFromElements(sma, e, lop, mu, period, is_prograde);
}

OrbitPos OrbitGetPosition(const Orbit* orbit, time_type time) {
    OrbitPos res = {0};
    res.M = (time - orbit->period) * OrbitGetMeanMotion(orbit);
    if (orbit->sma > 0) {
        res.M = fmod(res.M, PI*2);
        res.θ = _Mean2True(res.M, orbit->ecc);
    } else {
        res.θ = _Mean2TrueHyp(res.M, orbit->ecc);
    }
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    res.r = p / (1 + orbit->ecc * cos(res.θ));
    // printf("M = %f ; E = %f ; θ = %f r = %f\n", M, E, θ, r);
    // printf("pos.x %f pos.y 5f\n", postion->x, postion->y);
    res.longuitude = res.θ + orbit->lop;
    res.cartesian = FromPolar(res.r, res.longuitude);
    return res;
}

Vector2 OrbitGetVelocity(const Orbit* orbit, OrbitPos pos) {
    double cot_ɣ = -orbit->ecc * sin(pos.θ) / (1 + orbit->ecc * cos(pos.θ));
    Vector2 local_vel = Vector2Scale(
        Vector2Normalize((Vector2) {cot_ɣ, 1}),
         sqrt((2*orbit->mu) / pos.r - orbit->mu/orbit->sma) * (orbit->prograde ? -1 : 1)
    );
    return Vector2Rotate(local_vel, -pos.θ - orbit->lop);
}

time_type OrbitGetTimeUntilFocalAnomaly(const Orbit* orbit, double θ, time_type start_time) {
    // TODO for hyperbolic orbits
    if (orbit->sma < 0) {
        NOT_IMPLEMENTED
    }
    double mean_motion_inv = sqrt(orbit->sma*orbit->sma*orbit->sma / orbit->mu);  // s
    double period = 2 * PI * mean_motion_inv;
    double M = _True2Mean(θ, orbit->ecc);
    double reference_time = orbit->period + M *mean_motion_inv;
    time_type diff = fmod(reference_time - start_time, period);
    if (diff < 0) diff += period;
    return diff;
}

double OrbitGetMeanMotion(const Orbit* orbit) {
    return sqrt(orbit->mu / (fabs(orbit->sma)*orbit->sma*orbit->sma)) * (orbit->prograde ? 1.0 : -1.0);
}

time_type OrbitGetPeriod(const Orbit* orbit) {
    if (orbit->sma < 0) return INFINITY;
    return 2 * PI * sqrt(orbit->sma*orbit->sma*orbit->sma / orbit->mu);
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
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = (double) i / (double) buffer_size * PI * 2.0;
        double r = p / (1 + orbit->ecc*cos(θ));
        buffer[i] = FromPolar(r, θ + orbit->lop);
    }

    buffer[buffer_size - 1] = buffer[0];
}

void SampleOrbitWithOffset(const Orbit* orbit, Vector2* buffer, int buffer_size, double offset) {
    if (orbit->sma < 0) {
        SampleOrbitBounded(orbit, -acos(-1/orbit->ecc), acos(-1/orbit->ecc), buffer, buffer_size, offset);
    } else {
        SampleOrbitBounded(orbit, 0, 2*PI, buffer, buffer_size, offset);
        buffer[buffer_size - 1] = buffer[0];
    }
}

void SampleOrbitBounded(const Orbit* orbit, double θ_1, double θ_2, Vector2* buffer, int buffer_size, double offset) {
    if (orbit->prograde && θ_2 < θ_1) θ_2 += PI * 2.0;
    if (!orbit->prograde && θ_2 > θ_1) θ_2 -= PI * 2.0;
    double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = Lerp(θ_1, θ_2, (double) i / (double) (buffer_size - 1));
        double r = p / (1 + orbit->ecc*cos(θ));
        double cot_ɣ = -orbit->ecc * sin(θ) / (1 + orbit->ecc * cos(θ));
        Vector2 normal = Vector2Rotate(Vector2Normalize((Vector2) {1, -cot_ɣ}), θ + orbit->lop);
        buffer[i] = Vector2Add(FromPolar(r, θ + orbit->lop), Vector2Scale(normal, offset));
    }
}

#ifndef ORBIT_BUFFER_SIZE
#define ORBIT_BUFFER_SIZE 64
#endif
static Vector2 orbit_draw_buffer[ORBIT_BUFFER_SIZE];

void DrawOrbit(const Orbit* orbit, Color color) {
    SampleOrbit(orbit, orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}

void DrawOrbitWithOffset(const Orbit* orbit, double offset, Color color) {
    SampleOrbitWithOffset(orbit, orbit_draw_buffer, ORBIT_BUFFER_SIZE, offset);
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}

void DrawOrbitBounded(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2, double offset, Color color) {
    SampleOrbitBounded(orbit, bound1.θ, bound2.θ, orbit_draw_buffer, ORBIT_BUFFER_SIZE, offset);
    if (ORBIT_BUFFER_SIZE >= 2) {
        orbit_draw_buffer[0] = bound1.cartesian;
        orbit_draw_buffer[ORBIT_BUFFER_SIZE-1] = bound2.cartesian;
    }
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}
