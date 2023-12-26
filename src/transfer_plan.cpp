#include "transfer_plan.hpp"
#include "planet.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "ui.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "string_builder.hpp"


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
    case 0: return (α - sin(α) - β + sin(β)) / (x*x*x);             // no focus
    case 1: return (2*PI - (α - sin(α)) - β + sin(β)) / (x*x*x);    // F1 & F2
    case 2: return (α - sin(α) + β - sin(β)) / (x*x*x);             // F1
    case 3: return (2*PI - (α - sin(α)) + β - sin(β)) / (x*x*x);    // F2
    case 4: return (sinh(α) - α - sinh(β) + β) / (x*x*x);
    case 5: return (sinh(α) - α + sinh(β) - β) / (x*x*x);
    }
    FAIL("Solution provided to lambert solver must be 0, 1, 2, 3, 4 or 5")
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
    default: FAIL("newtown's method only implemented for cases 4 and 5")
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
    resource_transfer = ResourceTransfer();
}

void TransferPlanSolveInputImpl(TransferPlan* tp, const Orbit* from_orbit, const Orbit* to_orbit) {
    ASSERT_ALOMST_EQUAL_FLOAT(from_orbit->mu, to_orbit->mu)
    ASSERT(tp->arrival_time > tp->departure_time)
    double mu = from_orbit->mu;

    timemath::Time t1 = tp->departure_time;
    timemath::Time t2 = tp->arrival_time;
    OrbitPos pos1 = from_orbit->GetPosition(t1);
    OrbitPos pos2 = to_orbit->GetPosition(t2);
    
    double c = Vector2Distance(pos1.cartesian, pos2.cartesian);
    double r_sum = pos1.r + pos2.r;
    double a_min = 0.25 * (r_sum + c);
    double K = sqrt(fmax(r_sum - c, 0) / (r_sum + c));
    double y = timemath::Time::SecDiff(t2, t1) * sqrt(mu / (a_min*a_min*a_min));

    bool is_ellipse[2] =  {y > _Lambert(1e-3, K, 0), y > _Lambert(1e-3, K, 2)};
    bool first_solution[2] = { y < _Lambert(1, K, 0), y < _Lambert(1, K, 2) };

    double aa[2];

    tp->num_solutions = 2;
    for (int i=0; i < 2; i++) {
        if (is_ellipse[i]) {
            int lambert_case = (first_solution[i] ? 0 : 1) + i*2;
            double x = _SolveLambertBetweenBounds(y, K, 1e-5, 1.0, lambert_case);
            aa[i] = a_min / (x*x);
        } else {
            int lambert_case = i + 4;
            double x;
            if (y > _Lambert(-2, K, i + 4)) {
                x = _SolveLambertBetweenBounds(y, K, -2, 1e-5, lambert_case);
            } else {
                x = _SolveLambertWithNewton(y, K, lambert_case);
            }
            aa[i] = -a_min / (x*x);
        }
    }

    // Verify (for ellipses)

    for (int i=0; i < tp->num_solutions; i++) {
        //double r1_r2_outer_prod = Determinant(pos1.cartesian, pos2.cartesian);
        // Direct orbit is retrograde
        bool is_prograde;
        if (is_ellipse[i]) {
            bool direct_solution = timemath::Time::SecDiff(t2, t1) < PI * sqrt(fabs(aa[i])*aa[i]*aa[i] / mu);
            is_prograde = direct_solution ^ (i == 1);
        } else {
            is_prograde = i == 1;
        }
        tp->transfer_orbit[i] = Orbit(pos1, pos2, t1, aa[i], mu, is_prograde, i == 1);
        //OrbitPrint(&tp->transfer_orbit[i]); printf("\n");

        OrbitPos pos1_tf = tp->transfer_orbit[i].GetPosition(t1);
        OrbitPos pos2_tf = tp->transfer_orbit[i].GetPosition(t2);

        //SHOW_V2(OrbitGetVelocity(&tp->transfer_orbit[i], pos1_tf))
        //SHOW_V2(OrbitGetVelocity(&tp->from->orbit, pos1))
        tp->departure_dvs[i] = Vector2Subtract(tp->transfer_orbit[i].GetVelocity(pos1_tf), from_orbit->GetVelocity(pos1));
        tp->arrival_dvs[i] = Vector2Subtract(to_orbit->GetVelocity(pos2), tp->transfer_orbit[i].GetVelocity(pos2_tf));

        //ASSERT_ALOMST_EQUAL_FLOAT(pos1_.r, pos1.r)
        //ASSERT_ALOMST_EQUAL_FLOAT(pos2_.r, pos2.r)
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

void TransferPlanSetBestDeparture(TransferPlan* tp, timemath::Time t0, timemath::Time t1) {
    // TODO
    // simple optimization of dv vs departure time

    TransferPlan tp_copy = TransferPlan(*tp);
    timemath::Time xl = t0;
    timemath::Time xr = t1;
    
    tp_copy.departure_time = xl;
    TransferPlanSolve(&tp_copy);
    double yl = tp_copy.tot_dv;
    
    tp_copy.departure_time = xr;
    TransferPlanSolve(&tp_copy);
    double yr = tp_copy.tot_dv;

    const double CONVERGANCE_DELTA = 1e-10;
    const int MAX_ITER = 100;
    const double PHI_INV = 0.6180339887;
    for (int i = 0; yr - yl > CONVERGANCE_DELTA && i < MAX_ITER; i++) {
        if (yl < yr) {
            xr = xl + (PHI_INV) * (xr - xl).Seconds();
            
            tp_copy.departure_time = xr;
            TransferPlanSolve(&tp_copy);
            yr = tp_copy.tot_dv;
        } else {
            xl = xl + (1 - PHI_INV) * (xr - xl).Seconds();
            
            tp_copy.departure_time = xl;
            TransferPlanSolve(&tp_copy);
            yl = tp_copy.tot_dv;
        }
    }
    if (xl.IsInvalid() && xr.IsInvalid()) return;
    if (xl.IsInvalid()) tp->departure_time = yl;
    else if (xr.IsInvalid()) tp->departure_time = yr;
    else tp->departure_time = yl < yr ? xl : xr;

}

void TransferPlanSoonest(TransferPlan* tp, double dv_limit) {
    // TODO
    // least arrival time in departure_time x arrival_time where (dv = dvlimit)
}

void TransferPlan::Serialize(DataNode* data) const {
    data->SetI("resource_transfer_id", resource_transfer.resource_id);
    data->SetI("resource_transfer_qtt", resource_transfer.quantity);
    data->SetI("fuel_mass", fuel_mass);
    data->SetI("departure_planet", (int)departure_planet);
    data->SetI("arrival_planet", (int)arrival_planet);
    departure_time.Serialize(data->SetChild("departure_time", DataNode()));
    arrival_time.Serialize(data->SetChild("arrival_time", DataNode()));
    data->SetI("primary_solution", primary_solution);
}

void TransferPlan::Deserialize(const DataNode* data) {
    resource_transfer.resource_id = (ResourceType) data->GetI("resource_transfer_id", resource_transfer.resource_id);
    resource_transfer.quantity = data->GetI("resource_transfer_qtt", resource_transfer.quantity);
    fuel_mass = data->GetI("fuel_mass", fuel_mass);
    departure_planet = (entity_id_t) data->GetI("departure_planet", (int)departure_planet);
    arrival_planet = (entity_id_t) data->GetI("arrival_planet", (int)arrival_planet);
    departure_time.Deserialize(data->GetChild("departure_time"));
    arrival_time.Deserialize(data->GetChild("arrival_time"));
    primary_solution = data->GetI("primary_solution", primary_solution);

    GetPlanet(departure_planet);
    HohmannTransfer(  // Initialize hohmann_departure_time & hohmann_arrival_time
        &GetPlanet(departure_planet)->orbit, 
        &GetPlanet(arrival_planet)->orbit, 
        departure_time, &hohmann_departure_time, &hohmann_arrival_time, 
        NULL, NULL
    );
    TransferPlanSolve(this);
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
            printf("Error for x=%f, K=%f, solution=%d d/dx expected to be %f, but is measured to be %f\n", x, K, solution, ddx, central_difference);
            return 1;
        }
    }

    // build profile
#if true
    TransferPlan tp;
    tp.departure_planet = 1;  // Encelladus
    tp.arrival_planet = 2;    // Thetys

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
    FILE* f = fopen("dev_output/encelladus_thetys_landscape.csv", "w");
    fprintf(f, "departure, arrival, dv\n");
    for (tp.departure_time = start; tp.departure_time < opt_departure + 1; tp.departure_time = tp.departure_time + (opt_departure - start) / timemath::Time(30.0)) {
        for (tp.arrival_time = tp.departure_time; tp.arrival_time < opt_arrival + 1; tp.arrival_time = tp.arrival_time + (opt_arrival - start) / timemath::Time(30.0)) {
            TransferPlanSolveInputImpl(&tp, &orbit1, &orbit2);
            for (int i=0; i < tp.num_solutions; i++) {
                tp.dv1[i] = Vector2Length(tp.departure_dvs[i]);
                tp.dv2[i] = Vector2Length(tp.arrival_dvs[i]);
            }
            TransferPlanRankSolutions(&tp);

            fprintf(f, "%f, %f, %f\n", 
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

 void TransferPlanUI::Make() {
    plan = NULL;
    ship = GetInvalidId();
    is_dragging_departure    = false;
    is_dragging_arrival      = false;
    departure_handle_pos     = {0};
    arrival_handle_pos       = {0};
    redraw_queued            = false;
    departure_time_automatic = true;
    time_bounds[0]           = 0;
}

void TransferPlanUI::Abort() {
    if (IsIdValid(ship)) {
        GetShip(ship)->CloseEditedTransferPlan();
    }
    Make();
}

void TransferPlanUI::Update() {
    if (!IsActive()) {
        return;
    }

    Ship* ship_instance = GetShip(ship);

    if (IsIdValid(plan->departure_planet) && IsIdValid(plan->arrival_planet) && redraw_queued) {
        if (departure_time_automatic) {
            TransferPlanSetBestDeparture(plan, time_bounds[0], plan->arrival_time - 1000);
        }
        TransferPlanSolve(plan);
        is_valid = plan->num_solutions > 0 && plan->tot_dv <= ship_instance->GetCapableDV();
        is_valid = is_valid && GlobalGetNow() < plan->departure_time;

        redraw_queued = false;
    }

    if (is_valid) {
        if (plan->resource_transfer.resource_id == RESOURCE_NONE) {
            SetLogistics(0, ship_instance->GetFuelRequiredEmpty(plan->tot_dv));
        } else {
            resource_count_t payload = ship_instance->GetRemainingPayloadCapacity(plan->tot_dv);
            SetLogistics(payload, ship_instance->GetRemainingPayloadCapacity(0) - payload);
        }
    } else {
        SetLogistics(0, 0);
    }

    if (IsKeyPressed(KEY_ENTER) && is_valid) {
        ship_instance->ConfirmEditedTransferPlan();
        Make();
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
    OrbitPos from_pos = orbit->GetPosition(from);
    OrbitPos to_pos = orbit->GetPosition(to);

    int full_orbits = floor(timemath::Time::SecDiff(to, from) / orbit->GetPeriod().Seconds());
    double offset_per_pixel = GetScreenTransform()->InvTransformS(1);
    for (int i=1; i <= full_orbits; i++) {
        orbit->DrawWithOffset(offset_per_pixel * -3 * i, color);
    }
    orbit->DrawBounded(from_pos, to_pos, offset_per_pixel*3, color);
}

void _DrawTransferOrbit(const TransferPlan* plan, int solution, bool is_secondary, timemath::Time t0) {
    const Planet* from = GetPlanet(plan->departure_planet);
    const Planet* to = GetPlanet(plan->arrival_planet);
    Color velocity_color = YELLOW;
    Color orbit_color = Palette::transfer_ui;
    if (is_secondary) {
        velocity_color = ColorTint(velocity_color, GRAY);
        orbit_color = ColorTint(orbit_color, GRAY);
    }
    OrbitPos pos1 = plan->transfer_orbit[solution].GetPosition(plan->departure_time);
    OrbitPos pos2 = plan->transfer_orbit[solution].GetPosition(plan->arrival_time);
    _DrawSweep(&from->orbit, t0, plan->departure_time, orbit_color);
    _DrawSweep(&to->orbit,   t0, plan->arrival_time,   orbit_color);
    plan->transfer_orbit[solution].DrawBounded(pos1, pos2, 0, orbit_color);
}

timemath::Time _DrawHandle(
    const CoordinateTransform* c_transf, Vector2 pos, const Orbit* orbit, 
    timemath::Time current, timemath::Time t0, bool* is_dragging
) {
    Color c = Palette::blue;

    Vector2 radial_dir = Vector2Normalize(c_transf->InvTransformV(pos));
    radial_dir.y = -radial_dir.y;
    Vector2 tangent_dir = Vector2Rotate(radial_dir, PI/2);

    Vector2 node_pos = Vector2Add(pos, Vector2Scale(radial_dir, 20));
    Vector2 text_pos = Vector2Add(pos, Vector2Scale(radial_dir, 30));
    Vector2 plus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, -23));
    Vector2 minus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, 23));

    timemath::Time period = orbit->GetPeriod();
    int full_orbits = floor(timemath::Time::SecDiff(current, t0) / period.Seconds());
    if (full_orbits > 0) {
        char text_content[4];
        sprintf(text_content, "%+3d", full_orbits);
        DrawTextAligned(text_content, text_pos, TEXT_ALIGNMENT_HCENTER | TEXT_ALIGNMENT_RIGHT, c);
    }
    if (DrawTriangleButton(pos, Vector2Scale(radial_dir, 20), 10, c) & BUTTON_STATE_FLAG_JUST_PRESSED) {
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
    ButtonStateFlags button_state_plus  = DrawCircleButton(plus_pos, 10, c);
    ButtonStateFlags button_state_minus = DrawCircleButton(minus_pos, 10, c);
    if (button_state_plus & BUTTON_STATE_FLAG_JUST_PRESSED)  current = current + period;
    if (button_state_minus & BUTTON_STATE_FLAG_JUST_PRESSED) current = current - period;
    HandleButtonSound(button_state_plus);
    HandleButtonSound(button_state_minus);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        *is_dragging = false;
    }
    if (*is_dragging) {
        timemath::Time t0_2 = t0 + full_orbits * period.Seconds();

        Vector2 mouse_pos_world = GetMousePositionInWorld();
        double longuitude = atan2(mouse_pos_world.y, mouse_pos_world.x);
        double θ = longuitude - orbit->lop;
        current = t0_2 + orbit->GetTimeUntilFocalAnomaly(θ, t0_2);
        ASSERT(t0 < current);
    }
    return current;
}

void TransferPlanUI::Draw(const CoordinateTransform* c_transf) {
    if (!IsActive()) {
        return;
    }
    const Planet* from = GetPlanet(plan->departure_planet);
    const Planet* to = GetPlanet(plan->arrival_planet);

    departure_handle_pos = c_transf->TransformV(from->orbit.GetPosition(plan->departure_time).cartesian);
    arrival_handle_pos = c_transf->TransformV(to->orbit.GetPosition(plan->arrival_time).cartesian);

    timemath::Time new_departure_time = _DrawHandle(c_transf, departure_handle_pos, &from->orbit, plan->departure_time, time_bounds[0], &is_dragging_departure);
    timemath::Time new_arrival_time = _DrawHandle(c_transf, arrival_handle_pos, &to->orbit, plan->arrival_time, time_bounds[0], &is_dragging_arrival);
    if (time_bounds[0] < new_departure_time && new_departure_time < plan->arrival_time) {
        plan->departure_time = new_departure_time;
        redraw_queued = true;
    }
    if (plan->departure_time < new_arrival_time) {
        plan->arrival_time = new_arrival_time;
        redraw_queued = true;
    }

    if (plan->num_solutions == 1) {
        _DrawTransferOrbit(plan, plan->primary_solution, false, time_bounds[0]);
    } else if (plan->num_solutions == 2) {
        _DrawTransferOrbit(plan, plan->primary_solution, false, time_bounds[0]);
        //_DrawTransferOrbit(plan, 1 - plan->primary_solution, true, time_bounds[0]);
    }
}

void TransferPlanUI::DrawUI() {
    if (!IsActive()) {
        return;
    }
    
    Ship* ship_instance = GetShip(ship);
    
    const int y_margin = 5+50;
    UIContextCreate(
        GetScreenWidth() - 20*16 - 5, y_margin,
        20*16, MinInt(200, GetScreenHeight()) - 2*5 - y_margin, 
        16, Palette::transfer_ui
    );

    UIContextCurrent().Enclose(2, 2, Palette::bg, is_valid ? Palette::transfer_ui : Palette::red);
    
    StringBuilder sb = StringBuilder();
    sb.Add("Departs in ").AddTime(plan->departure_time - time_bounds[0]);
    sb.Add("\nArrives in ").AddTime(plan->arrival_time - time_bounds[0]);
    DebugPrintText("%i", sb.CountLines());
    UIContextPushInset(0, 18 * sb.CountLines() + 5);
    UIContextWrite(sb.c_str);
    UIContextFillline(
        fmin(timemath::Time::SecDiff(plan->arrival_time, time_bounds[0]) / timemath::Time::SecDiff(plan->hohmann_arrival_time, time_bounds[0]), 1.0), 
        Palette::transfer_ui, Palette::bg
    );
    UIContextPop();  // Inset
    sb.Clear();

    double total_dv = plan->dv1[plan->primary_solution] + plan->dv2[plan->primary_solution];
    sb.AddFormat("DV Tot    %5.3f km/s\n", total_dv/1000.0);
    resource_count_t capacity = ship_instance->GetRemainingPayloadCapacity(total_dv);
    resource_count_t max_capacity = ship_instance->GetRemainingPayloadCapacity(0);
    double capacity_ratio = (float)capacity / (float)max_capacity;

    if (capacity >= 0) {
        sb.AddFormat("Payload cap.  %d / %d", capacity, max_capacity);
    } else {
        sb.Add("Cannot make transfer");
    }

    UIContextPushInset(0, 18 * sb.CountLines() + 5);
    UIContextWrite(sb.c_str);
    UIContextFillline(fmax(0, capacity_ratio), capacity >= 0 ? Palette::transfer_ui : Palette::red, Palette::bg);
    UIContextPop();  // Inset

    int w = UIContextCurrent().width;
    UIContextPushInset(0, 20);
    {
        UIContextPushHSplit(0, w/3);
        ButtonStateFlags button_state = UIContextAsButton();
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        if(button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            TransferPlanSoonest(plan, ship_instance->GetCapableDV() - 1);
        }
        UIContextWrite("ASAP");
        UIContextPop();  // HSplit
    }
    {
        UIContextPushHSplit(w/3, 2*w/3);
        ButtonStateFlags button_state = UIContextAsButton();
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        if(button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            if (is_valid) {
                ship_instance->ConfirmEditedTransferPlan();
                Make();
            }
        }
        UIContextWrite("Confirm");
        UIContextPop();  // HSplit
    }
    {
        UIContextPushHSplit(2*w/3, w);
        ButtonStateFlags button_state = UIContextAsButton();
        if(button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            departure_time_automatic = !departure_time_automatic;
        }
        if (departure_time_automatic || (button_state & BUTTON_STATE_FLAG_HOVER)) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        UIContextWrite("Lock");
        UIContextPop();  // HSplit
    }
    UIContextPop();  // Inset
}

void TransferPlanUI::SetPlan(TransferPlan* pplan, entity_id_t pship, timemath::Time pmin_time, timemath::Time pmax_time) {
    if (is_dragging_departure || is_dragging_arrival) {
        return;
    }
    ship = pship;
    plan = pplan;
    if (plan == NULL) {
        return;
    }
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
    }
}

 void TransferPlanUI::SetResourceType(ResourceType resource_type) {
    if (plan == NULL) return;
    plan->resource_transfer.resource_id = resource_type;
}

 void TransferPlanUI::SetLogistics(resource_count_t payload_mass, resource_count_t fuel_mass) {
    if (plan == NULL) return;
    plan->resource_transfer.quantity = payload_mass;
    plan->fuel_mass = fuel_mass;
}

 void TransferPlanUI::SetDestination(entity_id_t planet) {
    if (is_dragging_departure || is_dragging_arrival || !IsIdValid(ship) || !IsIdValid(plan->departure_planet)) {
        return;
    }
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