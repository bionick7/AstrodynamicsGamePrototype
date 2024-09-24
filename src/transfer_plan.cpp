#include "transfer_plan.hpp"
#include "planet.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "ui.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "string_builder.hpp"
#include "render_utils.hpp"
#include "debug_console.hpp"

double _Lambert(double x, double K, int solution) {
    // x² = a_min/a
    // y = t_f * sqrt(mu / (a_min*a_min*a_min))
    if (fabs(x) < 1e-3 && solution != 1 && solution != 3) {
        // approaching singularity
        // Instead return Taylor expansion up to O(x³)
        double K2 = K*K;
        switch (solution) {
        case 0: return 4.0/3.0 * (1 - K2*K) + 0.4 * (1 - K2*K2*K) * x*x;
        case 2: return 4.0/3.0 * (1 + K2*K) + 0.4 * (1 + K2*K2*K) * x*x;
        case 4: return 4.0/3.0 * (1 - K2*K) - 0.4 * (1 - K2*K2*K) * x*x;
        case 5: return 4.0/3.0 * (1 + K2*K) - 0.4 * (1 + K2*K2*K) * x*x;
        }
    }
    double α, β;
    if (solution < 4) {
        α = 2 * asin(x);
        β = 2 * asin(K * x);
    } else {
        α = 2 * asinh(x);
        β = 2 * asinh(K * x);
    }
    switch (solution) {
    case 0: return         (α - sin(α)  - β + sin(β)) / (x*x*x);    // no focus
    case 1: return (2*PI - (α - sin(α)) - β + sin(β)) / (x*x*x);    // F1 & F2
    case 2: return         (α - sin(α)  + β - sin(β)) / (x*x*x);    // F1
    case 3: return (2*PI - (α - sin(α)) + β - sin(β)) / (x*x*x);    // F2
    case 4: return         (sinh(α) - α - sinh(β) + β) / (x*x*x);
    case 5: return         (sinh(α) - α + sinh(β) - β) / (x*x*x);
    }
    FAIL("Solution provided to lambert solver must be 0, 1, 2, 3, 4 or 5")
    return 0;
}

double _LambertDerivative(double x, double K, int solution) {
    // x² = a_min/a
    // y = t_f * sqrt(mu / (a_min*a_min*a_min))
    double α = 2 * asinh(x);
    double β = 2 * asinh(K * x);
    double dαdx = 2 / sqrt(x*x + 1);
    double dβdx = 2*K / sqrt(x*x*K*K + 1);
    switch (solution) {
    case 4: return (cosh(α)*dαdx - dαdx - cosh(β)*dβdx + dβdx) / (x*x*x) - (sinh(α) - α - sinh(β) + β) * 3 / (x*x*x*x);
    case 5: return (cosh(α)*dαdx - dαdx + cosh(β)*dβdx - dβdx) / (x*x*x) - (sinh(α) - α + sinh(β) - β) * 3 / (x*x*x*x);
    }
    FAIL("Derivative only implemented for cases 4 and 5")
    return 0;
}

double _SolveLambertBetweenBounds(double y, double K, double xl, double xr, int solution) {
    double yl = _Lambert(xl, K, solution) - y;
    double yr = _Lambert(xr, K, solution) - y;
    if (abs(yl) < 1e-6) {
        return xl;
    }
    if (abs(yr) < 1e-6) {
        return xr;
    }
    if (yl*yr > 0.0) {FAIL("yl = %f and yr = %f have the same sign (y = %f, xl = %f, xr = %f, K = %f, sol = %d\n)", yl, yr, y, xl, xr, K, solution)}
    //ASSERT(yl*yr < 0.0)
    int counter = 0;
    double xm, ym;
    do {
        xm = (xr + xl) / 2.0;
        ym = _Lambert(xm, K, solution) - y;
        if (ym * yr < 0.0) xl = xm;
        else xr = xm;
        if (counter++ > 1000) FAIL("Counter exceeded")
    } while (fabs(ym) > 1e-6 * y);
    
    double test_y = _Lambert(xm, K, solution);
    ASSERT_ALOMST_EQUAL_FLOAT(test_y, y)
    ASSERT(!isnan(xm))
    return xm;
}

double _SolveLambertWithNewton(double y, double K, int solution) {
    double xs;
    switch (solution) {
    case 4: xs = 2 * (K*K - 1) / y; break;
    case 5: xs = -2 * (K*K + 1) / y; break;
    default: FAIL("Newtown's method only implemented for cases 4 and 5")
    }

    int counter = 0;
    double ys, dydx_s;
    do {
        ys = _Lambert(xs, K, solution) - y;
        dydx_s = _LambertDerivative(xs, K, solution);
        xs -= ys / dydx_s;
        if (counter++ > 1000) FAIL("Counter exceeded y=%f K=%f, solution=%d, x settled at %f with dx %f ", y, K, solution, xs, ys / dydx_s)
    } while (fabs(ys) > 1e-6);
    
    double test_y = _Lambert(xs, K, solution);
    ASSERT_ALOMST_EQUAL_FLOAT(test_y, y)
    ASSERT(!isnan(xs))
    return xs;
}

TransferPlan::TransferPlan() {
    departure_planet = GetInvalidId();
    arrival_planet = GetInvalidId();
    num_solutions = 0;
    primary_solution = 0;
    for(int i=0; i < resources::MAX; i++) resource_transfer[i] = 0;
    fuel = 0;
}

void TransferPlanSolveInputImpl(TransferPlan* tp, const Orbit* from_orbit, const Orbit* to_orbit) {
    ASSERT_ALOMST_EQUAL_FLOAT(from_orbit->mu, to_orbit->mu)
    ASSERT(tp->arrival_time > tp->departure_time)
    double mu = from_orbit->mu;

    timemath::Time t1 = tp->departure_time;
    timemath::Time t2 = tp->arrival_time;
    OrbitPos pos1 = from_orbit->GetPosition(t1);
    OrbitPos pos2 = to_orbit->GetPosition(t2);
    
    double c = (pos1.cartesian - pos2.cartesian).Length();
    double r_sum = pos1.r + pos2.r;
    double a_min = 0.25 * (r_sum + c);
    double K = sqrt(fmax(r_sum - c, 0) / (r_sum + c));
    double y = timemath::Time::SecDiff(t2, t1) * sqrt(mu / (a_min*a_min*a_min));

    bool is_ellipse[2] =  {y > _Lambert(1e-3, K, 0), y > _Lambert(1e-3, K, 2)};
    bool first_solution[2] = { y < _Lambert(1, K, 0), y < _Lambert(1, K, 2) };

    double aa[2];
    int lambert_cases[2];

    tp->num_solutions = 2;
    for (int i=0; i < 2; i++) {
        if (is_ellipse[i]) {
            lambert_cases[i] = (first_solution[i] ? 0 : 1) + i*2;
            double x = _SolveLambertBetweenBounds(y, K, 1e-5, 1.0, lambert_cases[i]);
            aa[i] = a_min / (x*x);
        } else {
            lambert_cases[i] = i + 4;
            double x;
            if (y > _Lambert(-2, K, i + 4)) {
                x = _SolveLambertBetweenBounds(y, K, -2, 1e-5, lambert_cases[i]);
            } else {
                x = _SolveLambertWithNewton(y, K, lambert_cases[i]);
            }
            aa[i] = -a_min / (x*x);
        }
    }

    // Verify (for ellipses)

    for (int i=0; i < tp->num_solutions; i++) {
        //double r1_r2_outer_prod = Determinant(pos1.cartesian, pos2.cartesian);
        // Direct orbit is retrograde
        //is_prograde = lambert_cases[i] == 0 || lambert_cases[i] == 2;
        tp->transfer_orbit[i] = Orbit(pos1, pos2, t1, aa[i], mu, lambert_cases[i]);

        OrbitPos pos1_tf = tp->transfer_orbit[i].GetPosition(t1);
        OrbitPos pos2_tf = tp->transfer_orbit[i].GetPosition(t2);

        //SHOW_V2(OrbitGetVelocity(&tp->transfer_orbit[i], pos1_tf))
        //SHOW_V2(OrbitGetVelocity(&tp->from->orbit, pos1))
        tp->departure_dvs[i] = (tp->transfer_orbit[i].GetVelocity(pos1_tf) - from_orbit->GetVelocity(pos1));
        tp->arrival_dvs[i] = (to_orbit->GetVelocity(pos2) - tp->transfer_orbit[i].GetVelocity(pos2_tf));


        //DEBUG_SHOW_F(pos2_tf.r / pos2.r)

        //ASSERT_ALOMST_EQUAL_FLOAT(pos1_tf.r, pos1.r)
        //ASSERT_ALOMST_EQUAL_FLOAT(pos2_tf.r, pos2.r)
    }
}

void TransferPlanRankSolutions(TransferPlan* tp) {
    if (tp->num_solutions == 1) {
        tp->tot_dv = tp->dv1[0] + tp->dv2[0];
        tp->tot_dv_sec = 0;
        tp->primary_solution = 0;
    } else if (tp->num_solutions == 2) {
        tp->tot_dv = tp->dv1[0] + tp->dv2[0];
        tp->tot_dv_sec = tp->dv1[1] + tp->dv2[1];
        if (tp->tot_dv < tp->tot_dv_sec) {
            tp->primary_solution = 0;
        } else {
            tp->primary_solution = 1;
            std::swap(tp->tot_dv, tp->tot_dv_sec);
        }
    }
}

void TransferPlanSolve(TransferPlan* tp) {
    ASSERT(IsIdValid(tp->departure_planet))
    ASSERT(IsIdValid(tp->arrival_planet))
    const Planet* from = GetPlanet(tp->departure_planet);
    const Planet* to = GetPlanet(tp->arrival_planet);

    TransferPlanSolveInputImpl(tp, &from->orbit, &to->orbit);
    
    for (int i=0; i < tp->num_solutions; i++) {
        tp->dv1[i] = from->GetDVFromExcessVelocity(tp->departure_dvs[i]);
        tp->dv2[i] = to->GetDVFromExcessVelocity(tp->arrival_dvs[i]);
    }

    TransferPlanRankSolutions(tp);
}

double TransferPlanSolveWithTimes(TransferPlan tp, timemath::Time departure, timemath::Time arrival) {
    tp.departure_time = departure;
    tp.arrival_time = arrival;
    TransferPlanSolve(&tp);
    double res = tp.tot_dv;
    return res;
}

void TransferPlanSetBestDeparture(TransferPlan* tp, timemath::Time t0, timemath::Time t1) {
    // simple optimization of dv vs departure time

    const double H = 1.0;

    timemath::Time x = tp->departure_time;

    double diff_seconds;
    int counter = 0;
    do {
        double dv_0 = TransferPlanSolveWithTimes(*tp, x, tp->arrival_time);
        double dv_plus = TransferPlanSolveWithTimes(*tp, x + H, tp->arrival_time);
        double dv_minus = TransferPlanSolveWithTimes(*tp, x - H, tp->arrival_time);

        double deriv = (dv_plus - dv_minus) / (2 * H);
        double deriv2 = (dv_plus - 2 * dv_0 + dv_minus) / (H*H);
        
        diff_seconds = deriv / deriv2 * 0.5;
        //diff_seconds = deriv * 1000;
        //DebugPrintText("%f, D %f, D2 %f => dt: %f", dv_0, deriv, deriv2, diff_seconds);
        if (std::abs(diff_seconds) > (t1 - t0).Seconds() / 10.0) {
            diff_seconds = (t1 - t0).Seconds() / 10.0 * (diff_seconds > 0 ? 1 : -1);
        }
        x = x - timemath::Time(diff_seconds);
    } while (abs(diff_seconds) > 1 && counter++ < 100);
    if (x.IsInvalid()) {
        return;
    }    
    if (x > t1 - 1) x = t1 - 1;
    if (x < t0 + 1) x = t0 + 1;
    tp->departure_time = x;
}

void TransferPlanSoonest(TransferPlan* tp, double dv_limit, timemath::Time earliest) {
    // least arrival time in departure_time x arrival_time where (dv = dvlimit)
    // Simplified imperfect (but quick) solution:
    //    assume departure time is equal to hohmann departure time
    //    adjust arrival time

    timemath::Time hohmann_departure_time, hohmann_arrival_time;
    double hohmann_departure_dv, hohmann_arrival_dv;
    
    HohmannTransfer(
        &GetPlanet(tp->departure_planet)->orbit,
        &GetPlanet(tp->arrival_planet)->orbit, earliest,
        &hohmann_departure_time, &hohmann_arrival_time,
        &hohmann_departure_dv, &hohmann_arrival_dv
    );

    timemath::Time xl = hohmann_arrival_time;
    double yl = hohmann_departure_dv + hohmann_arrival_dv - dv_limit;
    
    // TODO: Finding a good starting guess that is generally applicable is tricky
    timemath::Time xr = hohmann_departure_time + (hohmann_arrival_time - hohmann_departure_time) * 0.1;
    double yr = TransferPlanSolveWithTimes(*tp, hohmann_departure_time, xr) - dv_limit;

    ASSERT(yl < 0)
    ASSERT(yr > 0)

    for (int i=0; i < 100; i++) {
        timemath::Time xm = xl + (xr - xl) / 2;
        double ym = TransferPlanSolveWithTimes(*tp, hohmann_departure_time, xm) - dv_limit;
        if (ym > 0) {
            yr = ym;
            xr = xm;
        } else {
            yl = ym;
            xl = xm;
        }
        if (yl > -1) {
            break;
        }
    }

    tp->departure_time = hohmann_departure_time;
    tp->arrival_time = xl;
}

void TransferPlan::Serialize(DataNode* data) const {
    data->SerializeBuffer("resource_transfer", resource_transfer, resources::names, resources::MAX);
    //data->SetI("fuel_mass", fuel_mass);
    data->SetI("departure_planet", departure_planet.AsInt());
    data->SetI("arrival_planet", arrival_planet.AsInt());
    departure_time.Serialize(data, "departure_time");
    arrival_time.Serialize(data, "arrival_time");
    //data->SetI("primary_solution", primary_solution);
}

void TransferPlan::Deserialize(const DataNode* data) {
    data->DeserializeBuffer("resource_transfer", resource_transfer, resources::names, resources::MAX);
    //fuel_mass = data->GetI("fuel_mass", fuel_mass);
    departure_planet = RID(data->GetI("departure_planet", departure_planet.AsInt()));
    arrival_planet = RID(data->GetI("arrival_planet", arrival_planet.AsInt()));
    departure_time.Deserialize(data, "departure_time");
    arrival_time.Deserialize(data, "arrival_time");
    //primary_solution = data->GetI("primary_solution", primary_solution);

    GetPlanet(departure_planet);
    HohmannTransfer(  // Initialize hohmann_departure_time & hohmann_arrival_time
        &GetPlanet(departure_planet)->orbit, 
        &GetPlanet(arrival_planet)->orbit, 
        departure_time, &hohmann_departure_time, &hohmann_arrival_time, 
        NULL, NULL
    );
    TransferPlanSolve(this);
}

resource_count_t TransferPlan::GetPayloadMass() const {
    resource_count_t total_payload = 0;
    for(int i=0; i < resources::MAX; i++) {
        total_payload += resource_transfer[i];
    }
    return total_payload;
}

TransferPlanCycle::~TransferPlanCycle() {
    delete[] planets;
    delete[] dvs;
    delete[] resource_transfers;
}

void TransferPlanCycle::Serialize(DataNode *data) const {
    data->CreatChildArray("cycle", stops);
    for(int i=0; i < stops; i++) {
        DataNode* cycle_stop_data = data->InsertIntoChildArray("cycles", i);
        cycle_stop_data->Set("planet", GetPlanet(planets[i])->name.GetChar());
        cycle_stop_data->SetF("dv", dvs[i]);
        cycle_stop_data->SerializeBuffer("resources", resource_transfers[i], resources::names, resources::MAX);
    }
}

void TransferPlanCycle::Deserialize(const DataNode *data) {
    delete[] planets;
    delete[] dvs;
    delete[] resource_transfers;

    stops = data->GetChildArrayLen("cycle");
    if (stops == 0) return;

    planets = new RID[stops];
    dvs = new double[stops];
    resource_transfers = new resource_count_t[stops + 1][resources::MAX];

    for(int i=0; i < stops; i++) {
        const DataNode* cycle_stop_data = data->GetChildArrayElem("cycle", i);
        RID planet_id = GetGlobalState()->GetFromStringIdentifier(cycle_stop_data->GetArrayElem("planet", i));
        if (!IsIdValidTyped(planet_id, EntityType::PLANET)) {
            planets[i] = GetInvalidId();    
            // Means planet IDs can be invalid
            FAIL("No such planet: '%s'", cycle_stop_data->GetArrayElem("planet", i))
            continue;
        }
        planets[i] = planet_id;
        dvs[i] = cycle_stop_data->GetF("dv");
        cycle_stop_data->DeserializeBuffer("resources", resource_transfers[i], resources::names, resources::MAX);
    }
}

void TransferPlanCycle::GenFromTransferplans(TransferPlan *transferplans, int transferplan_count) {
    if (transferplans[0].departure_planet != transferplans[transferplan_count-1].arrival_planet) {
        FAIL("Tried to create invalid Transferplan Cycle: final arrival planet did not match departure planet")
        return;
    }
    
    delete[] planets;
    delete[] dvs;
    delete[] resource_transfers;

    stops = transferplan_count;

    planets = new RID[stops];
    dvs = new double[stops];
    resource_transfers = new resource_count_t[stops + 1][resources::MAX];
    
    for(int i=0; i < transferplan_count; i++) {
        planets[i] = transferplans[i].arrival_planet;
        dvs[i] = transferplans[i].tot_dv;
        for(int j=0; j < resources::MAX; j++) {
            resource_transfers[i][j] = transferplans[i].resource_transfer[j];
        }
    }
}

void TransferPlanCycle::Reset() {
    delete[] planets;
    delete[] dvs;
    delete[] resource_transfers;
    
    planets = NULL;
    dvs = NULL;
    resource_transfers  = NULL;

    stops = 0;
}

int TransferPlanTests() {
    // Lambert derivatives
    const double epsilon = 1e-5;
    for (double K = 0.0; K < 1.0; K += 0.2) 
        for (double x = -5.0; x < -0.1; x += 0.1) 
            for (int solution = 4; solution <= 5; solution++) 
    {
        double ddx = _LambertDerivative(x, K, solution);
        double central_difference = (_Lambert(x + epsilon, K, solution) - _Lambert(x - epsilon, K, solution)) / (2*epsilon);
        if (fabs(ddx - central_difference) > epsilon) {
            ERROR("Error for x=%f, K=%f, solution=%d d/dx expected to be %f, but is measured to be %f\n", x, K, solution, ddx, central_difference);
            return 1;
        }
    }

    // Construct sample transfer
    TransferPlan tp;
    tp.departure_planet = RID(1, EntityType::PLANET);  // Enceladus
    tp.arrival_planet = RID(2, EntityType::PLANET);    // Thetys

    Orbit orbit1 = Orbit(
       237.905e+6,
       0, 0, G * 568.336e+24,
       0, true
    );
    Orbit orbit2 = Orbit(
       294.619e+6,
       0, 0, G * 568.336e+24,
       0, true
    );

    timemath::Time start = 0;
    timemath::Time opt_departure = 0;
    timemath::Time opt_arrival = 0;
    double best_dv1 = 0;
    double best_dv2 = 0;
    HohmannTransfer(&orbit1, &orbit2, start, &opt_departure, &opt_arrival, &best_dv1, &best_dv2);
    
    //TransferPlanSoonest(&tp, 2e3);

    // build profile
#if false
    FILE* f = fopen("dev/encelladus_thetys_landscape.csv", "w");
    fprintf(f, "departure,arrival,dv\n");
    for (tp.departure_time = start; tp.departure_time < opt_departure + 1; tp.departure_time = tp.departure_time + (opt_departure - start) / timemath::Time(30.0)) {
        for (tp.arrival_time = tp.departure_time; tp.arrival_time < opt_arrival + 1; tp.arrival_time = tp.arrival_time + (opt_arrival - start) / timemath::Time(30.0)) {
            TransferPlanSolveInputImpl(&tp, &orbit1, &orbit2);
            for (int i=0; i < tp.num_solutions; i++) {
                tp.dv1[i] = tp.departure_dvs[i].Length();
                tp.dv2[i] = tp.arrival_dvs[i].Length();
            }
            TransferPlanRankSolutions(&tp);

            fprintf(f, "%f,%f,%f\n", 
                tp.departure_time.Seconds(), 
                tp.arrival_time.Seconds(),
                tp.tot_dv
            );
        }
    }

    fprintf(f, "%f, %f, %f\n", 
        opt_departure, 
        opt_arrival,
        best_dv1 + best_dv2
    );

    fclose(f);

#endif
    return 0;
}

TransferPlanUI::TransferPlanUI() { 
    Reset();
    departure_time_automatic = false;
}

void TransferPlanUI::Reset() {
    plan = NULL;
    ship = GetInvalidId();
    is_dragging_departure = false;
    is_dragging_arrival   = false;
    departure_handle_pos  = Vector2Zero();
    arrival_handle_pos    = Vector2Zero();
    redraw_queued         = false;
    time_bounds[0]        = 0;
}

void TransferPlanUI::Abort() {
    if (IsIdValid(ship)) {
        GetShip(ship)->CloseEditedTransferPlan();
    }
    Reset();
}

void TransferPlanUI::Update() {
    if (!IsActive()) {
        return;
    }

    Ship* ship_instance = GetShip(ship);

    if (IsIdValid(plan->departure_planet) && IsIdValid(plan->arrival_planet) && redraw_queued) {
        if (departure_time_automatic) {
            TransferPlanSetBestDeparture(plan, time_bounds[0], plan->arrival_time - timemath::Time(60));
        }
        TransferPlanSolve(plan);
        double required_dv = plan->tot_dv;
        bool ship_can_aerobrake = ship_instance->CountModulesOfClass(GetShipModules()->expected_modules.heatshield) > 0;
        if (GetPlanet(plan->arrival_planet)->has_atmosphere && ship_can_aerobrake) {
            // Is overriding tot_dv a good idea?
            plan->tot_dv = plan->dv1[plan->primary_solution];
        }
        //DebugPrintText("%s has atmosphere ? %s", GetPlanet(plan->arrival_planet)->name.GetChar(), GetPlanet(plan->arrival_planet)->has_atmosphere ? "y" : "n");
        is_valid = plan->num_solutions > 0 && plan->tot_dv <= ship_instance->GetCapableDV();
        is_valid = is_valid && GlobalGetNow() < plan->departure_time;

        redraw_queued = false;
    }

    // Update Fuel
    if (is_valid) {
        resource_count_t total_payload = plan->GetPayloadMass();
        resource_count_t capacity = ship_instance->GetRemainingPayloadCapacity(plan->tot_dv);
        if (total_payload > capacity) total_payload = capacity;
        SetLogistics(ship_instance->GetFuelRequired(plan->tot_dv, total_payload));
        //DebugPrintText("%d fuel (%d payload)", ship_instance->GetFuelRequired(plan->tot_dv, total_payload), total_payload);

    } else {
        SetLogistics(0);
    }

    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_ENTER) && is_valid) {
        ship_instance->ConfirmEditedTransferPlan();
        Reset();
    }
}

void _TransferPlanInitialize(TransferPlan* tp, timemath::Time t0) {
    // Sets departure and arrival time to the dv-cheapest values (assuming hohmann transfer)
    ASSERT(IsIdValid(tp->departure_planet))
    ASSERT(IsIdValid(tp->arrival_planet))
    
    HohmannTransfer(
        &GetPlanet(tp->departure_planet)->orbit, 
        &GetPlanet(tp->arrival_planet)->orbit, 
        t0, &tp->hohmann_departure_time, &tp->hohmann_arrival_time, 
        NULL, NULL
    );
    tp->departure_time = tp->hohmann_departure_time;
    tp->arrival_time = tp->hohmann_arrival_time;

    // StringBuilder sb;
    // sb.AddI(tp->departure_planet).Add(", ").AddI(tp->arrival_planet).Add(" ; ");
    // sb.AddDate(tp->departure_time).Add(", ").AddDate(tp->arrival_time);
    // INFO(sb.c_str)
}

void _DrawSweep(const Orbit* orbit, timemath::Time from, timemath::Time to, Color color) {

    int full_orbits = floor(timemath::Time::SecDiff(to, from) / orbit->GetPeriod().Seconds());
    double offset_per_pixel = 0;//GetCoordinateTransform()->InvTransformS(1);
    for (int i=1; i <= full_orbits; i++) {
        orbit->DrawWithOffset(offset_per_pixel * -3 * i, color);
    }
    //OrbitPos from_pos = orbit->GetPosition(from);
    //OrbitPos to_pos = orbit->GetPosition(to);
    //OrbitSegment segment = OrbitSegment(orbit, from_pos, to_pos);
    //RenderOrbit(&segment, OrbitRenderMode::Solid, color);  // TODO: re-introduce offset
}

void _DrawTransferOrbit(const TransferPlan* plan, int solution, bool is_secondary, timemath::Time t0) {
    const Planet* from = GetPlanet(plan->departure_planet);
    const Planet* to = GetPlanet(plan->arrival_planet);
    Color velocity_color = YELLOW;
    Color orbit_color = Palette::ally;
    if (is_secondary) {
        velocity_color = ColorTint(velocity_color, GRAY);
        orbit_color = ColorTint(orbit_color, GRAY);
    }
    OrbitPos pos1 = plan->transfer_orbit[solution].GetPosition(plan->departure_time);
    OrbitPos pos2 = plan->transfer_orbit[solution].GetPosition(plan->arrival_time);
    _DrawSweep(&from->orbit, t0, plan->departure_time, orbit_color);
    _DrawSweep(&to->orbit, t0, plan->arrival_time, orbit_color);
    OrbitSegment segment = OrbitSegment(&plan->transfer_orbit[solution], pos1, pos2);
    RenderOrbit(&segment, orbit_render_mode::Solid, orbit_color);
}

timemath::Time _DrawHandle(
    Vector2 pos, Vector2 radial_dir, const Orbit* orbit, 
    timemath::Time current, timemath::Time t0, bool* is_dragging
) {
    Color c = Palette::interactable_main;

    Vector2 tangent_dir = Vector2Rotate(radial_dir, PI/2);
    Vector2 node_pos = Vector2Add(pos, Vector2Scale(radial_dir, 20));
    Vector2 text_pos = Vector2Add(pos, Vector2Scale(radial_dir, 30));
    Vector2 plus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, -23));
    Vector2 minus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, 23));

    timemath::Time period = orbit->GetPeriod();
    int full_orbits = floor(timemath::Time::SecDiff(current, t0) / period.Seconds());
    if (full_orbits > 0) {
        static char text_content[5];
        sprintf(text_content, "%+3d", full_orbits);
        DrawTextAligned(text_content, text_pos, text_alignment::HCENTER | text_alignment::RIGHT, 
                        c, Palette::bg, 0);
    }
    if (DrawTriangleButton(pos, Vector2Scale(radial_dir, 20), 10, c) & button_state_flags::JUST_PRESSED) {
        *is_dragging = true;
    }
    const int extend = 5;
    DrawLineV(
        Vector2Subtract(minus_pos, {extend, 0}),
        Vector2Add(minus_pos, {extend, 0}),
        c
    );
    DrawLineV(
        Vector2Subtract(plus_pos, {extend, 0}),
        Vector2Add(plus_pos, {extend, 0}),
        c
    );
    DrawLineV(
        Vector2Subtract(plus_pos, {0, extend}),
        Vector2Add(plus_pos, {0, extend}),
        c
    );
    button_state_flags::T button_state_plus  = DrawCircleButton(plus_pos, 10, c);
    button_state_flags::T button_state_minus = DrawCircleButton(minus_pos, 10, c);
    if (button_state_plus & button_state_flags::JUST_PRESSED)  current = current + period;
    if (button_state_minus & button_state_flags::JUST_PRESSED) current = current - period;
    HandleButtonSound(button_state_plus);
    HandleButtonSound(button_state_minus);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        *is_dragging = false;
    }
    if (*is_dragging) {
        timemath::Time t0_2 = t0 + full_orbits * period.Seconds();
        Vector2 local_mouse_pos = orbit->GetMousPosOnPlane();
        float focal_anomaly = -atan2f(local_mouse_pos.y, local_mouse_pos.x);
        current = t0_2 + orbit->GetTimeUntilFocalAnomaly(focal_anomaly, t0_2);
        ASSERT(t0 < current);
    }
    return current;
}

void TransferPlanUI::Draw3D() {
    if (!IsActive()) {
        return;
    }
    
    if (plan->num_solutions == 1) {
        _DrawTransferOrbit(plan, plan->primary_solution, false, time_bounds[0]);
    } else if (plan->num_solutions == 2) {
        _DrawTransferOrbit(plan, plan->primary_solution, false, time_bounds[0]);
        if (GetSettingBool("transferplan_draw_second_solution")) {
            _DrawTransferOrbit(plan, 1 - plan->primary_solution, true, time_bounds[0]);
        }
    }
}

void TransferPlanUI::Draw3DGizmos() {
    if (!IsActive()) {
        return;
    }
    
    const Planet* from = GetPlanet(plan->departure_planet);
    const Planet* to = GetPlanet(plan->arrival_planet);

    departure_handle_pos = GetCamera()->GetScreenPos(from->orbit.GetPosition(plan->departure_time).cartesian);
    arrival_handle_pos = GetCamera()->GetScreenPos(to->orbit.GetPosition(plan->arrival_time).cartesian);
    
    Vector2 handle_radial_dir = {0.0f, -1.0f};
    timemath::Time new_departure_time = _DrawHandle(
        departure_handle_pos, handle_radial_dir, &from->orbit, plan->departure_time, 
        time_bounds[0], &is_dragging_departure
    );
    timemath::Time new_arrival_time = _DrawHandle(
        arrival_handle_pos, handle_radial_dir, &to->orbit, plan->arrival_time, 
        time_bounds[0], &is_dragging_arrival
    );

    if (time_bounds[0] < new_departure_time && new_departure_time < plan->arrival_time) {
        plan->departure_time = new_departure_time;
        redraw_queued = true;
    }
    if (plan->departure_time < new_arrival_time) {
        plan->arrival_time = new_arrival_time;
        redraw_queued = true;
    }
}

void TransferPlanUI::DrawUI() {
    if (IsSelectingDestination()) {
        StringBuilder sb = StringBuilder("Select Destination [RMB to cancel]");
        if (IsIdValidTyped(GetGlobalState()->hover, EntityType::PLANET)) {
            const Planet* hover_planet = GetPlanet(GetGlobalState()->hover);
            double dv = PlanetsMinDV(plan->departure_planet, GetGlobalState()->hover, false);
            sb.AddFormat("\n[LMB to select %s] (\u2265 %4.2f km/s \u0394V)", hover_planet->name.GetChar(), dv / 1e3);
        }
        //ui::SetMouseHint(sb.c_str);

        // Manually draw mousehint at z-level 0 to avaid interference with mouse cursor
        Vector2 txt_size = MeasureTextEx(GetFontDefault(), sb.c_str, DEFAULT_FONT_SIZE, 1);
        ui::PushMouseHint(GetMousePosition(), txt_size.x + 8, txt_size.y + 8, 0);
        ui::Enclose();
        ui::Write(sb.c_str);
        ui::Pop();
    }

    if (!IsActive()) {
        return;
    }
    
    // Panel
    
    Ship* ship_instance = GetShip(ship);
    
    ui::Enclose();
    
    StringBuilder sb = StringBuilder();
    sb.Add("Departs in ").AddTime(plan->departure_time - time_bounds[0]);
    sb.Add("\nArrives in ").AddTime(plan->arrival_time - time_bounds[0]);
    //DebugPrintText("%i", sb.CountLines());
    ui::PushInset((DEFAULT_FONT_SIZE) * sb.CountLines() + 1);
    ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
    ui::FillLine(
        fmin(timemath::Time::SecDiff(plan->arrival_time, time_bounds[0]) / timemath::Time::SecDiff(plan->hohmann_arrival_time, time_bounds[0]), 1.0), 
        Palette::ui_main, Palette::bg
    );
    ui::Pop();  // Inset
    sb.Clear();

    double total_dv = plan->tot_dv;
    sb.AddFormat("\u0394V Tot    %5.3f km/s", total_dv/1000.0);
    double savings = plan->dv1[plan->primary_solution] + plan->dv2[plan->primary_solution] - total_dv;
    if (savings != 0) {
        sb.AddFormat("  [%5.3f km/s saved]\n", savings/1000.0);
    } else {
        sb.Add("\n");
    }
    resource_count_t capacity = ship_instance->GetRemainingPayloadCapacity(total_dv);
    resource_count_t max_capacity = ship_instance->GetRemainingPayloadCapacity(0);
    double capacity_ratio = (float)capacity / (float)max_capacity;

    if (capacity >= 0) {
        sb.AddFormat("Payload cap.  %d / %d", capacity, max_capacity);
    } else {
        sb.Add("Cannot make transfer");
    }

    ui::PushInset((DEFAULT_FONT_SIZE) * sb.CountLines() + 1);
    ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
    ui::FillLine(fmax(0, capacity_ratio), capacity >= 0 ? Palette::ui_main : Palette::red, Palette::bg);
    ui::Pop();  // Inset

    int w = ui::Current()->width;
    ui::PushInset(DEFAULT_FONT_SIZE+4);
    {
        ui::PushHSplit(0, w/3);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EncloseEx(0, Palette::bg, Palette::ui_main, 4);
        }
        if(button_state & button_state_flags::JUST_PRESSED) {
            TransferPlanSoonest(plan, ship_instance->GetCapableDV() - 1, GlobalGetNow());
        }
        ui::WriteEx("ASAP", text_alignment::CENTER, false);
        ui::Pop();  // HSplit
    }
    {
        ui::PushHSplit(w/3, 2*w/3);
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            ui::EncloseEx(0, Palette::bg, Palette::ui_main, 4);
            resource_count_t tot_payload = plan->GetPayloadMass();
            if (tot_payload == 0 && GetShip(ship)->GetShipType() == ship_type::TRANSPORT) {
                ui::SetMouseHint("WARNING: Transferring with a transport ship\nwithout resources");
            }
        }
        if(button_state & button_state_flags::JUST_PRESSED) {
            if (is_valid) {
                ship_instance->ConfirmEditedTransferPlan();
                Reset();
            }
        }
        ui::WriteEx("Confirm", text_alignment::CENTER, false);
        ui::Pop();  // HSplit
    }
    {
        ui::PushHSplit(2*w/3, w);
        button_state_flags::T button_state = ui::AsButton();
        if (departure_time_automatic || (button_state & button_state_flags::HOVER)) {
            ui::EncloseEx(0, Palette::bg, Palette::ui_main, 4);
        }
        if(button_state & button_state_flags::JUST_PRESSED) {
            departure_time_automatic = !departure_time_automatic;
        }
        ui::WriteEx("Lock", text_alignment::CENTER, false);
        ui::Pop();  // HSplit
    }
    ui::Pop();  // Inset
}

void TransferPlanUI::SetPlan(TransferPlan* pplan, RID pship, timemath::Time pmin_time, timemath::Time pmax_time) {
    if (is_dragging_departure || is_dragging_arrival) {
        return;
    }
    ship = pship;
    plan = pplan;
    if (plan == NULL) {
        return;
    }
    plan->fuel_type = GetShipClassByRID(GetShip(ship)->ship_class)->fuel_resource;
    /*Ship& ship_comp = GetShip(ship);
    if (ship_comp.confirmed_plans_count > 0) {
        time_bounds[0] = ship_comp.prepared_plans[ship_comp.confirmed_plans_count - 1].arrival_time;
        SHOW_F(ship_comp.prepared_plans[ship_comp.confirmed_plans_count - 1].departure_time)
        SHOW_F(ship_comp.prepared_plans[ship_comp.confirmed_plans_count - 1].arrival_time)
    } else {
        time_bounds[0] = GlobalGetNow();
    }*/
    time_bounds[0] = pmin_time;
    time_bounds[1] = pmax_time;
    if (IsIdValid(plan->departure_planet) && IsIdValid(plan->arrival_planet)) {
        _TransferPlanInitialize(plan, time_bounds[0]);
        redraw_queued = true;
    }
}

void TransferPlanUI::SetLogistics(resource_count_t fuel_mass) {
    if (plan == NULL) return;
    plan->fuel = fuel_mass;
}

void TransferPlanUI::SetDestination(RID planet) {
    if (is_dragging_departure || is_dragging_arrival || !IsIdValid(ship) || !IsIdValid(plan->departure_planet)) {
        return;
    }
    SetPlanetTabIndex(0);  // Force planets to show resources
    plan->arrival_planet = planet;
    if (
           plan->departure_planet != GetInvalidId() 
        && plan->arrival_planet   != GetInvalidId() 
        && plan->departure_planet != plan->arrival_planet
    ) {
        _TransferPlanInitialize(plan, time_bounds[0]);
    }
}

bool TransferPlanUI::IsSelectingDestination() {
    if (plan == NULL) return false;
    if (!IsIdValid(ship)) return false;
    if (!IsIdValid(plan->departure_planet)) return false;
    if (IsIdValid(plan->arrival_planet)) return false;
    return true;
}

bool TransferPlanUI::IsActive() {
    if (plan == NULL) return false;
    if (!IsIdValid(ship)) return false;
    if (!IsIdValid(plan->departure_planet)) return false;
    if (!IsIdValid(plan->arrival_planet)) return false;
    return true;
}
