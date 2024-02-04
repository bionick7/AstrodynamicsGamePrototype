#include "astro.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "coordinate_transform.hpp"
#include "logging.hpp"
#include "global_state.hpp"

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
    periapsis_dir = DVector3(
        cos(longuitude_of_periapsis), 0,
        sin(longuitude_of_periapsis)
    );
    normal = is_prograde ? DVector3::Up() : DVector3::Down();
    this->epoch = epoch;
}

Orbit::Orbit(double semi_major_axis, double eccenetricity, double inclination, 
             double longuitude_of_periapsis, double right_ascention_of_ascending_node, 
             double mu, timemath::Time epoch) {
    this->mu = mu;
    sma = semi_major_axis;
    ecc = eccenetricity;

    DVector3 ascending_node = DVector3(
        cos(right_ascention_of_ascending_node), 0,
        sin(right_ascention_of_ascending_node)
    );
    normal = DVector3::Up().Rotated(ascending_node, inclination);
    periapsis_dir = ascending_node.Rotated(normal, longuitude_of_periapsis);
    this->epoch = epoch;
}

Orbit::Orbit(OrbitPos pos1, OrbitPos pos2, timemath::Time time_at_pos1, 
             double sma, double mu, bool cut_focus, bool is_prograde) {
    // https://en.wikipedia.org/wiki/Lambert%27s_problem
    // goes from pos1 to pos2
    //if (!is_prograde) Swap(&pos1, &pos2);
    DVector3 r1_r2_cross = pos1.cartesian.Cross(pos2.cartesian);
    double d = (pos1.cartesian - pos2.cartesian).Length() / 2.0;
    double A = (pos2.r - pos1.r) / 2;
    double E = d / A;
    double B_sqr = d*d - A*A;

    // focus of trajectory in canonical reference system of hyperbola
    double x_0 = - (pos2.r + pos1.r) / (2*E);
    double y_0 = sqrt(B_sqr * fmax(x_0*x_0 / (A*A) - 1, 0));

    // 2nd focus of trajectory in canonical reference system of hyperbola
    double x_f = x_0 + 2*sma / E;
    double y_f = sqrt(B_sqr * fmax(x_f*x_f / (A*A) - 1, 0));

    if (cut_focus) {  // For a hyperbola this means, it's the more direct way
        y_0 = -y_0;
    }
    if (r1_r2_cross.y < 0) {  // Indirect solution
        y_0 = -y_0;
        y_f = -y_f;
    }
    double ecc = hypot(x_0 - x_f, y_0 - y_f) / (2 * fabs(sma));
    ASSERT(!isnan(ecc))

    DVector3 canon_z = r1_r2_cross.Normalized();
    DVector3 canon_x = (pos2.cartesian - pos1.cartesian).Normalized();
    if (!is_prograde) {
        canon_z = -canon_z;
    }
    DVector3 canon_y = canon_z.Cross(canon_x);
    if (sma < 0) {
        canon_z = -canon_z;
    }
    DVector3 periapsis_dir = (canon_x * (x_0 - x_f) + canon_y * (y_0 - y_f)).Normalized();
    periapsis_dir = periapsis_dir * Sign(sma);

    //DVector3 center = (pos1.cartesian + pos2.cartesian) * .5;
    //DebugDrawLine(center + canon_x * x_0 + canon_y * y_0, center + canon_x * x_f + canon_y * y_f);
    //DebugDrawConic(pos1.cartesian, (pos1.cartesian - pos2.cartesian).Normalized() * E, DVector3::Up(), A);
    DebugDrawLine(DVector3::Zero(), pos1.cartesian);
    DebugDrawLine(DVector3::Zero(), canon_z * 2e8);
    DebugDrawLine(DVector3::Zero(), periapsis_dir * 2e8);

    double θ_1 = periapsis_dir.SignedAngleTo(pos1.cartesian, canon_z);
    double M_1 = sma < 0 ? _True2MeanHyp(θ_1, ecc) : _True2Mean(θ_1, ecc);

    DEBUG_SHOW_F(θ_1)
    DEBUG_SHOW_F(M_1)

    this->epoch = time_at_pos1 - timemath::Time(M_1 * sqrt(fabs(sma)*sma*sma / mu));
    //period = fmod(period, sqrt(sma*sma*sma / mu) * 2*PI);
    this->sma = sma;
    this->mu = mu;
    this->ecc = ecc;
    this->periapsis_dir = periapsis_dir;
    this->normal = canon_z;
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
        
    res.cartesian = res.r * periapsis_dir.Rotated(normal, res.θ);
    return res;
}

DVector3 Orbit::GetVelocity(OrbitPos pos) const {
    double cot_ɣ = -ecc * sin(pos.θ) / (1 + ecc * cos(pos.θ));
    double v = sqrt((2*mu) / pos.r - mu/sma);
    DVector3 local_vel = DVector3(cot_ɣ, 0, 1).Normalized() * v;
    return RadialToGlobal(local_vel, pos.cartesian / pos.r);
}

DVector3 Orbit::RadialToGlobal(DVector3 inp, DVector3 z) const {
    DVector3 local_coords_z = z;
    DVector3 local_coords_y = normal;
    DVector3 local_coords_x = local_coords_y.Cross(local_coords_z);
    return inp.Vector3Transform(local_coords_x, local_coords_y, local_coords_z, DVector3::Zero());
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

Vector2 Orbit::GetMousPosOnPlane() const {
    Ray mouse_ray = GetMouseRay(GetMousePosition(), GetCamera()->rl_camera);
    Matrix orbit_transform = MatrixFromColumns((Vector3) periapsis_dir, (Vector3) normal, (Vector3) periapsis_dir.Cross(normal));
    Matrix inv_orbit_transform = MatrixInvert(orbit_transform);
    mouse_ray.position = Vector3Transform(mouse_ray.position, inv_orbit_transform);
    mouse_ray.direction = Vector3Transform(mouse_ray.direction, inv_orbit_transform);
    float scale = mouse_ray.position.y / mouse_ray.direction.y;
    Vector3 crossing = Vector3Subtract(mouse_ray.position, Vector3Scale(mouse_ray.direction, scale));
    return { crossing.x, crossing.z };
}

double Orbit::GetMeanMotion() const {
    return sqrt(mu / (fabs(sma)*sma*sma));
}

timemath::Time Orbit::GetPeriod() const {
    if (sma < 0) return INFINITY;
    return 2 * PI * sqrt(sma*sma*sma / mu);
}

OrbitPos Orbit::FromFocal(double focal) const {
    OrbitPos res;
    res.θ = focal;

    if (ecc < 1) {
        res.M = _True2Mean(res.M, ecc);
        res.time = epoch + res.M / GetMeanMotion();
    } else {
        res.M = _True2MeanHyp(res.M, ecc);
        res.time = epoch + res.M / GetMeanMotion();
    }
    double p = sma * (1 - ecc*ecc);
    res.r = p / (1 + ecc * cos(res.θ));        
    res.cartesian = res.r * periapsis_dir.Rotated(normal, res.θ);
    return res;
}

void Orbit::Inspect() const {
    INFO("a = %f m, e = %f", sma, ecc);
}

void Orbit::Update(timemath::Time time, DVector3* position, DVector3* velocity) const {
    OrbitPos orbit_position = GetPosition(time);

    *position = orbit_position.cartesian;
    *velocity = GetVelocity(orbit_position);
}

void Orbit::SampleBounded(double θ_1, double θ_2, DVector3* buffer, int buffer_size, double offset) const {
    if (θ_2 < θ_1) θ_2 += PI * 2.0;
    double p = sma * (1 - ecc*ecc);
    for (int i=0; i < buffer_size; i++) {
        double θ = Lerp(θ_1, θ_2, (double) i / (double) (buffer_size - 1));
        double r = p / (1 + ecc*cos(θ));
        double cot_ɣ = -ecc * sin(θ) / (1 + ecc * cos(θ));
        DVector3 dir = periapsis_dir.Rotated(normal, θ);
        DVector3 normal = RadialToGlobal(DVector3(1, 0, cot_ɣ).Normalized(), dir);
        buffer[i] = r * dir + normal * offset;
    }
}

void Orbit::Draw(Color color) const {
    DrawWithOffset(0.0, color);
}

void Orbit::DrawWithOffset(double offset, Color color) const {
    OrbitPos begin = GetPosition(GlobalGetNow());
    OrbitPos end = begin;
    end.θ += 2 * PI;
}

OrbitSegment::OrbitSegment(const Orbit* orbit){
    this->orbit = orbit;
    this->is_full_circle = true;
    this->bound1 = orbit->FromFocal(0);
    this->bound2 = orbit->FromFocal(2*PI);
}

OrbitSegment::OrbitSegment(const Orbit* orbit, OrbitPos bound1, OrbitPos bound2){
    this->orbit = orbit;
    this->is_full_circle = false;
    this->bound1 = bound1;
    this->bound2 = bound2;
}

Vector2 _ConicEval2D(const Orbit* orbit, float focal) {
    float r = GameCamera::WorldToRender(orbit->sma) * (1 - orbit->ecc * orbit->ecc) / (1 + orbit->ecc * cos(focal));
    return {(float) r * cos(focal), (float) r * sin(focal)};
}

Vector2 OrbitSegment::ClosestApprox(Vector2 p, bool letExtrude) const {
    const float EPS = 1e-2f;
    float focal = atan2(p.y, p.x);
    if (!is_full_circle) {
        // Sdf for hyperbolas looks really weird and inaccurate, but in practice it's good where we need it to be
        focal = Clamp(focal, (float)bound1.θ, (float)bound2.θ);
    }
    Vector2 point1 = _ConicEval2D(orbit, focal - EPS);
    Vector2 point2 = _ConicEval2D(orbit, focal + EPS);
    Vector2 dp = Vector2Subtract(point2, point1);
    Vector2 p_rel = Vector2Subtract(p, point1);

    Vector2 projected = letExtrude
        ? Vector2Project(p_rel, dp)
        : Vector2Scale(dp, Clamp(Vector2DotProduct(dp, p_rel) / Vector2LengthSqr(dp), 0, 1));
    //Vector2 projected = (p - point1).Project(point2 - point1);
    return Vector2Add(point1, projected);
}

Vector2 OrbitSegment::AdjustmentToClosest(Vector2 p) const {
    // Approximate Unsigned distance field that is good at small distances, but breaks fast at alrger ones
    // Does this by approximating the tangent at the closest anomaly
    // Takes in *local* position and returns deviation from closest point on conic section

    // https://www.shadertoy.com/view/fdyfRK

    Vector2 closest;
    if (orbit->ecc >= 1) {
        p = Vector2Invert(p);  // need to invert for hyperbol for unknown reasons
        // requires prestep
        closest = ClosestApprox(p, true);
        Vector2 startPoint = _ConicEval2D(orbit, (float) bound1.θ);
        Vector2 endPoint = _ConicEval2D(orbit, (float) bound2.θ);
        Vector2 midPoint = Vector2Scale(Vector2Add(startPoint, endPoint), 0.5f);
        Vector2 closestClampPoint = Vector2DistanceSqr(startPoint, p) < Vector2DistanceSqr(endPoint, p) ? startPoint : endPoint;
        closest = Vector2DistanceSqr(closest, midPoint) < Vector2DistanceSqr(closestClampPoint, midPoint) ? closest : closestClampPoint;
    } else {
        closest = ClosestApprox(p, false);
    }
    return orbit->ecc < 1 ? Vector2Subtract(p, closest) : Vector2Subtract(closest, p);
}

OrbitPos OrbitSegment::TraceRay(Vector3 origin, Vector3 ray, float* distance, 
                                timemath::Time* mouseover_time, Vector3* local_mouseover_pos) const {
    Vector3 crossing = Vector3Subtract(origin, Vector3Scale(ray, origin.y / ray.y));
    Vector2 crossing2D = { crossing.x, crossing.z };
    Vector2 delta = AdjustmentToClosest(crossing2D);
    //delta = { delta.y, delta.x };
    *distance = Vector2Length(delta);
    float focal = PI/2 + atan2(crossing2D.y - delta.y, crossing2D.x - delta.x);
    OrbitPos res = orbit->FromFocal(focal);
    if (orbit->ecc > 1) {
        res = orbit->FromFocal(focal + PI);
    }
    *local_mouseover_pos = Vector3Subtract(crossing, { delta.x, 0, delta.y });
    
    if (orbit->ecc < 1) {
        timemath::Time minTime = GlobalGetNow();
        *mouseover_time = res.time + orbit->GetPeriod() * floor((minTime / orbit->GetPeriod()).Seconds());
        while (*mouseover_time < minTime)  // Should only happen once this time
            *mouseover_time = *mouseover_time + orbit->GetPeriod();
    } else {
        *mouseover_time = res.time;
    }
    return res;
}

void HohmannTransfer(const Orbit* from, const Orbit* to,
                     timemath::Time t0,
                     timemath::Time* departure, timemath::Time* arrival,
                     double* dv1, double* dv2
) {
    if (fabs(from->normal.Dot(to->normal)) < 0.8) {
        WARNING("Hohmann transfer only accurate if from and to are aligned")
    }
    // only accurate if from and to are aligned
    double mu = from->mu;
    double hohmann_a = (from->sma + to->sma) * 0.5;
    double hohmann_flight_time = sqrt(hohmann_a*hohmann_a*hohmann_a / mu) * PI;
    //DVector3 normal = (from->normal + to->normal).Normalized();
    DVector3 normal = DVector3::Up();
    double p1_mean_motion = from->GetMeanMotion() * Sign(normal.Dot(from->normal));
    double p2_mean_motion = to->GetMeanMotion() * Sign(normal.Dot(to->normal));
    double relative_mean_motion = p2_mean_motion - p1_mean_motion;
    double current_relative_annomaly = from->GetPosition(t0).cartesian.SignedAngleTo(to->GetPosition(t0).cartesian, normal);
    double target_relative_anomaly = PosMod(PI - p2_mean_motion * hohmann_flight_time, 2*PI);
    double departure_wait_time = (target_relative_anomaly - current_relative_annomaly) / relative_mean_motion;
    double relative_period = fabs(2 * PI / relative_mean_motion);
    departure_wait_time = PosMod(departure_wait_time, relative_period);
    if (departure != NULL) *departure = t0 + departure_wait_time;
    if (arrival   != NULL) *arrival = t0 + departure_wait_time + hohmann_flight_time;
    if (dv1       != NULL) *dv1 = fabs(sqrt(mu * (2 / from->sma - 1 / hohmann_a)) - sqrt(mu / from->sma));
    if (dv2       != NULL) *dv2 = fabs(sqrt(mu / to->sma) - sqrt(mu * (2 / to->sma - 1 / hohmann_a)));
}

void GetDVTable(StringBuilder* sb, bool include_arobreaks) {
    sb->Add("|          ");
    for(int j=0; j < GetPlanets()->GetPlanetCount(); j++) {
        sb->AddFormat("|%10s", GetPlanetByIndex(j)->name);
    }
    sb->Add("|\n");
    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        sb->AddFormat("|%10s", GetPlanetByIndex(i)->name);
        for(int j=0; j < GetPlanets()->GetPlanetCount(); j++) {
            if (i == j) {
                sb->Add("|          ");
                continue;
            }
            double dv1, dv2;
            HohmannTransfer(
                &GetPlanetByIndex(i)->orbit, &GetPlanetByIndex(j)->orbit,
                GlobalGetNow(), NULL, NULL, &dv1, &dv2
            );
            double tot_dv = GetPlanetByIndex(i)->GetDVFromExcessVelocity(dv1);
            if (!include_arobreaks || !GetPlanetByIndex(j)->has_atmosphere) {
                tot_dv += GetPlanetByIndex(j)->GetDVFromExcessVelocity(dv2);
            }
            sb->AddFormat("| %8.3f ", tot_dv * 1e-3);
        }
        sb->Add("|\n");
    }
}