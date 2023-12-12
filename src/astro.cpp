#include "astro.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "coordinate_transform.hpp"
#include "logging.hpp"

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

Orbit::Orbit() : Orbit(1, 0, 0, 1, 0, false) { }

Orbit::Orbit(double semi_major_axis, double eccenetricity, double longuitude_of_periapsis, double mu, timemath::Time epoch, bool is_prograde) {
    this->mu = mu;
    sma = semi_major_axis;
    ecc = eccenetricity;
    lop = longuitude_of_periapsis;
    this->epoch = epoch;
    prograde = is_prograde;
}

Orbit::Orbit(Vector2 pos, Vector2 vel, timemath::Time t, double mu) {
    double r = Vector2Length(pos);
    double energy = Vector2LengthSqr(vel) / 2 - mu / r;
    double ang_mom = fabs(pos.x*vel.y - pos.y*vel.x);
    Vector2 ecc_vector = Vector2Subtract(
        Vector2Scale(pos, Vector2LengthSqr(vel) / mu - 1.0 / r),
        Vector2Scale(vel, Vector2DotProduct(pos, vel) / mu)
    );
    ecc = Vector2Length(ecc_vector);
    sma = -mu / (2 * energy);
    lop = atan2(ecc_vector.y, ecc_vector.x) + ecc_vector.x > 0 ? PI : 0;
    double angular_pos = (atan2(pos.y, pos.x) + pos.x > 0 ? PI : 0);
    double mean_motion = sqrt(mu / (sma*sma*sma)) * (ang_mom > 0.0 ? 1.0 : -1.0);
    epoch = t + (angular_pos - lop) / mean_motion;
    this->mu = mu;
    prograde = ang_mom > 0;
}

Orbit::Orbit(OrbitPos pos1, OrbitPos pos2, timemath::Time time_at_pos1, double sma, double mu, bool is_prograde, bool cut_focus) {
    // https://en.wikipedia.org/wiki/Lambert%27s_problem
    // goes from pos1 to pos2
    //if (!is_prograde) Swap(&pos1, &pos2);
    double r1_r2_outer_prod = Determinant(pos1.cartesian, pos2.cartesian);
    double d = Vector2Distance(pos1.cartesian, pos2.cartesian) / 2.0;
    double A = (pos2.r - pos1.r) / 2;
    double E = d / A;
    double B_sqr = d*d - A*A;
    double x_0 = - (pos2.r + pos1.r) / (2*E);
    double y_0 = Sign(r1_r2_outer_prod) * sqrt(B_sqr * fmax(x_0*x_0 / (A*A) - 1, 0));
    double x_f = x_0 + 2*sma / E;
    double y_f = sqrt(B_sqr * fmax(x_f*x_f / (A*A) - 1, 0));
    if (cut_focus) y_f = -y_f;  // For a hyperbola this means, it's the more direct way
    ecc = hypot(x_0 - x_f, y_0 - y_f) / (2 * fabs(sma));
    ASSERT(!isnan(ecc))

    Vector2 canon_x = Vector2Normalize(Vector2Scale(Vector2Subtract(pos2.cartesian, pos1.cartesian), .5));
    Vector2 canon_y = Vector2Rotate(canon_x, PI/2);
    Vector2 periapsis_dir = Apply2DTransform({0}, canon_x, canon_y, {(float)(x_0 - x_f), (float)(y_0 - y_f)});
    periapsis_dir = Vector2Scale(periapsis_dir, Sign(sma));

    // DEBUG DRAWING

    lop = atan2(periapsis_dir.y, periapsis_dir.x);
    double θ_1 = pos1.longuitude - lop;
    double M_1 = sma < 0 ? _True2MeanHyp(θ_1, ecc) : _True2Mean(θ_1, ecc);
    // TODO: what happens if the orbit is retrograde

    epoch = time_at_pos1 - timemath::Time(M_1 * sqrt(fabs(sma)*sma*sma / mu) * (is_prograde ? 1.0 : -1.0));
    //period = fmod(period, sqrt(sma*sma*sma / mu) * 2*PI);
    this->sma = sma;
    this->mu = mu;
    this->prograde = is_prograde;
}

OrbitPos Orbit::GetPosition(timemath::Time time) const {
    OrbitPos res = {0};
    res.M = timemath::Time:: SecDiff(time, epoch) * GetMeanMotion();
    if (sma > 0) {
        res.M = fmod(res.M, PI*2);
        res.θ = _Mean2True(res.M, ecc);
    } else {
        res.θ = _Mean2TrueHyp(res.M, ecc);
    }
    double p = sma * (1 - ecc*ecc);
    res.r = p / (1 + ecc * cos(res.θ));
    //INFO("M = %f ; θ = %f r = %f", res.M, res.θ, res.r);
    //INFO("pos.x %f pos.y 5f", res.cartesian.x, res.cartesian.y);
    res.longuitude = res.θ + lop;
    res.cartesian = FromPolar(res.r, res.longuitude);
    return res;
}

Vector2 Orbit::GetVelocity(OrbitPos pos) const {
    double cot_ɣ = -ecc * sin(pos.θ) / (1 + ecc * cos(pos.θ));
    Vector2 local_vel = Vector2Scale(
        Vector2Normalize({(float) cot_ɣ, 1}),
         sqrt((2*mu) / pos.r - mu/sma) * (prograde ? -1 : 1)
    );
    return Vector2Rotate(local_vel, -pos.θ - lop);
}

timemath::Time Orbit::GetTimeUntilFocalAnomaly(double θ, timemath::Time start_time) const {
    // TODO for hyperbolic orbits
    if (sma < 0) {
        NOT_IMPLEMENTED
    }
    double mean_motion = GetMeanMotion();  // 1/s
    timemath::Time period = timemath::Time(fabs(2 * PI /mean_motion));
    double M = _True2Mean(θ, ecc);
    double M0 = fmod(timemath::Time::SecDiff(start_time, epoch) * mean_motion, 2.0*PI);
    timemath::Time diff = timemath::Time((M - M0) / mean_motion).PosMod(period);
    return diff;
}

double Orbit::GetMeanMotion() const {
    return sqrt(mu / (fabs(sma)*sma*sma)) * (prograde ? 1.0 : -1.0);
}

timemath::Time Orbit::GetPeriod() const {
    if (sma < 0) return INFINITY;
    return 2 * PI * sqrt(sma*sma*sma / mu);
}

void Orbit::Inspect() const {
    printf("%s a = %f m, e = %f, lop = %f", (prograde ? "" : "R"), sma, ecc, lop);
}

void Orbit::Update(timemath::Time time, Vector2* position, Vector2* velocity) const {
    OrbitPos orbit_position = GetPosition(time);

    *position = orbit_position.cartesian;
    *velocity = GetVelocity(orbit_position);
}

void Orbit::Sample(Vector2* buffer, int buffer_size) const {
    double p = sma * (1 - ecc*ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = (double) i / (double) buffer_size * PI * 2.0;
        double r = p / (1 + ecc*cos(θ));
        buffer[i] = FromPolar(r, θ + lop);
    }

    buffer[buffer_size - 1] = buffer[0];
}

void Orbit::SampleWithOffset(Vector2* buffer, int buffer_size, double offset) const {
    if (sma < 0) {
        SampleBounded(-acos(-1/ecc), acos(-1/ecc), buffer, buffer_size, offset);
    } else {
        SampleBounded(0, 2*PI, buffer, buffer_size, offset);
        buffer[buffer_size - 1] = buffer[0];
    }
}

void Orbit::SampleBounded(double θ_1, double θ_2, Vector2* buffer, int buffer_size, double offset) const {
    if (prograde && θ_2 < θ_1) θ_2 += PI * 2.0;
    if (!prograde && θ_2 > θ_1) θ_2 -= PI * 2.0;
    double p = sma * (1 - ecc*ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = Lerp(θ_1, θ_2, (double) i / (double) (buffer_size - 1));
        double r = p / (1 + ecc*cos(θ));
        double cot_ɣ = -ecc * sin(θ) / (1 + ecc * cos(θ));
        Vector2 normal = Vector2Rotate(Vector2Normalize({1, -(float)cot_ɣ}), θ + lop);
        buffer[i] = Vector2Add(FromPolar(r, θ + lop), Vector2Scale(normal, offset));
    }
}

#ifndef ORBIT_BUFFER_SIZE
#define ORBIT_BUFFER_SIZE 64
#endif
static Vector2 orbit_draw_buffer[ORBIT_BUFFER_SIZE];

void Orbit::Draw(Color color) const {
    Sample(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}

void Orbit::DrawWithOffset(double offset, Color color) const {
    SampleWithOffset(orbit_draw_buffer, ORBIT_BUFFER_SIZE, offset);
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}

void Orbit::DrawBounded(OrbitPos bound1, OrbitPos bound2, double offset, Color color) const {
    SampleBounded(bound1.θ, bound2.θ, orbit_draw_buffer, ORBIT_BUFFER_SIZE, offset);
    if (ORBIT_BUFFER_SIZE >= 2) {
        orbit_draw_buffer[0] = bound1.cartesian;
        orbit_draw_buffer[ORBIT_BUFFER_SIZE-1] = bound2.cartesian;
    }
    GetScreenTransform()->TransformBuffer(orbit_draw_buffer, ORBIT_BUFFER_SIZE);
    DrawLineStrip(&orbit_draw_buffer[0], ORBIT_BUFFER_SIZE, color);
}

void HohmannTransfer(const Orbit* from, const Orbit* to, timemath::Time t0, timemath::Time* departure, timemath::Time* arrival, double* dv1, double* dv2) {
    double mu = from->mu;
    double hohmann_a = (from->sma + to->sma) * 0.5;
    double hohmann_flight_time = sqrt(hohmann_a*hohmann_a*hohmann_a / mu) * PI;
    double p1_mean_motion = from->GetMeanMotion();
    double p2_mean_motion = to->GetMeanMotion();
    double relative_mean_motion = p2_mean_motion - p1_mean_motion;
    double current_relative_annomaly = to->GetPosition(t0).longuitude - from->GetPosition(t0).longuitude;
    double target_relative_anomaly = PosMod(PI - p2_mean_motion * hohmann_flight_time, 2*PI);
    double departure_wait_time = (target_relative_anomaly - current_relative_annomaly) / relative_mean_motion;
    double relative_period = fabs(2 * PI / relative_mean_motion);
    departure_wait_time = PosMod(departure_wait_time, relative_period);
    if (departure != NULL) *departure = t0 + departure_wait_time;
    if (arrival   != NULL) *arrival = t0 + departure_wait_time + hohmann_flight_time;
    if (dv1       != NULL) *dv1 = fabs(sqrt(mu * (2 / from->sma - 1 / hohmann_a)) - sqrt(mu / from->sma));
    if (dv2       != NULL) *dv2 = fabs(sqrt(mu / to->sma) - sqrt(mu * (2 / to->sma - 1 / hohmann_a)));
}
