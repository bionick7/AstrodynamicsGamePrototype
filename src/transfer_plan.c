#include "transfer_plan.h"
#include "global_state.h"
#include "debug_drawing.h"

double _sqr(double x) {return x*x;}

double _lambert(double x, double K, int solution) {
    // x² = a_min/a
    // y = t_f * sqrt(mu / (a_min*a_min*a_min))
    double α = 2 * asin(x);
    double β = 2 * asin(K * x);
    switch (solution) {
    case 0: return (α - sin(α) - β + sin(β)) / (x*x*x);             // no focus
    case 1: return (2*PI - (α - sin(α)) - β + sin(β)) / (x*x*x);    // F1 & F2
    case 2: return (α - sin(α) + β - sin(β)) / (x*x*x);             // F1
    case 3: return (2*PI - (α - sin(α)) + β - sin(β)) / (x*x*x);    // F2
    }
}

/*double _lambert_derivative(double x, double K, int solution) {
    // x² = a_min/a
    // y = t_f * sqrt(mu / (a_min*a_min*a_min))
    double α = 2 * asin(x);
    double β = 2 * asin(K * x);
    double dαdx = 2 / sqrt(1 - x*x);
    double dβdx = 2*K / sqrt(1 - x*x*K*K);
    switch (solution) {
    case 0: return (dαdx*(1 - cos(α)) - dβdx*(1 - cos(β)))* x - 6 * (α - sin(α) - β + sin(β));
    case 2: return (dαdx*(1 - cos(α)) + dβdx*(1 - cos(β)))* x - 6 * (α - sin(α) - β + sin(β));


    dαdx = 2 / sqrt(1 - x²)
    dβdx = 2 K / sqrt(1 - x²K²)
    dαdx (1 - cos(α)) - dβdx (1 - cos(β))
    d²αdx² (1 - cos(α)) + dαdx (1 + sin(α)) - dβdx (1 - cos(β))

    2(1 - cos(α)) / sqrt(1 - x²) - 2 K (1 - cos(β)) / sqrt(1 - x²K²)
    }
}*/

double _lerp_from_array(double t, double array[], int array_len) {
    int index_left = floorf(t * array_len);
    if (index_left < 0) index_left = 0;
    if (index_left >= array_len - 1) index_left = array_len - 2;
    int index_right = index_left + 1;
    double t_local = (t * array_len - index_left) / (index_right - index_left);
    if (t_local < 0.0) t_local = 0.0;
    if (t_local > 1.0) t_local = 1.0;
    return array[index_left] * t_local + array[index_right] * (1.0 - t_local);
}

double _solve_lambert_between_bounds(double y, double K, double xl, double xr, int solution) {
    double yl = _lambert(xl, K, solution) - y;
    double yr = _lambert(xr, K, solution) - y;
    //if (yl*yr > 0.0) {printf("yl = %f and yr = %f have the same sign (y = %f, xl = %f, xr = %f, K = %f, sol = %d\n)", yl, yr, y, xl, xr, K, solution);}
    ASSERT(yl*yr < 0.0)
    double xm, ym;
    do {
        xm = (xr + xl) / 2.0;
        ym = _lambert(xm, K, solution) - y;
        if (ym * yr < 0.0) xl = xm;
        else xr = xm;
    } while (fabs(ym) > 1e-6 * y);
    
    double test_y = _lambert(xm, K, solution);
    ASSERT_ALOMST_EQUAL(test_y, y)
    return xm;
}

void TransferPlanSolve(TransferPlan* tp) {
    ASSERT_ALOMST_EQUAL(tp->from->orbit.mu, tp->to->orbit.mu)
    double mu = tp->from->orbit.mu;

    double t1 = tp->departure_time;
    double t2 = tp->arrival_time;
    OrbitPos pos1 = OrbitGetPosition(&tp->from->orbit, t1);
    OrbitPos pos2 = OrbitGetPosition(&tp->to->orbit, t2);

    double c = Vector2Distance(pos1.cartesian, pos2.cartesian);
    double r_sum = pos1.r + pos2.r;
    double a_min = 0.25 * (r_sum + c);
    double K = sqrt((r_sum - c) / (r_sum + c));
    double y = (t2 - t1) * sqrt(mu / (a_min*a_min*a_min));

    //double curve1_min_x = _lerp_from_array(K, simplified_lambert_minimum_tabulation_case_0, SIMPLIFIED_LAMBERT_MINIMUM_TABULATION_SIZE);
    //double curve2_min_x = _lerp_from_array(K, simplified_lambert_minimum_tabulation_case_2, SIMPLIFIED_LAMBERT_MINIMUM_TABULATION_SIZE);
    double curve_min[2] = { _lambert(1e-3, K, 0), _lambert(1e-3, K, 2) };
    bool first_solution[2] = { y < _lambert(1, K, 0), y < _lambert(1, K, 2) };

    double aa[2];

    tp->num_solutions = 0;
    for (int i=0; i < 2; i++) {
        if (y > curve_min[i]) {
            int lambert_case = (first_solution[i] ? 0 : 1) + i*2;
            double x = _solve_lambert_between_bounds(y, K, 1e-3, 1.0, lambert_case);
            aa[i] = a_min / (x*x);
            tp->num_solutions++;
        } else {
            aa[i] = 0;
        }
    }
    // SHOW_F(aa[0]) SHOW_F(aa[1]) SHOW_F(aa[2]) SHOW_F(aa[3])

    // double α = 2 * asin(x);
    // double β = 2 * asin(K * x);
    // y = sqrt(α - sin(α) - β + sin(β)) / (x*x*x)
    // x = sqrt(a_min/a)
    // y = t_f * sqrt(µ / a_min³)

    // t_f * sqrt(µ / a_min³) = sqrt(α - sin(α) - β + sin(β)) / sqrt(a_min/a)³
    // t_f = = sqrt(α - sin(α) - β + sin(β)) * sqrt(a_min³ / µ) / sqrt(a_min/a)³
    // t_f = = sqrt(α - sin(α) - β + sin(β)) * sqrt(a³ / µ)

    //tp->num_solutions = 1;

    for (int i=0; i < tp->num_solutions; i++) {
        double α = 2 * asin(sqrt(a_min / aa[i]));
        double β = 2 * asin(K * sqrt(a_min / aa[i]));

        int lambert_case = (first_solution[i] ? 0 : 1) + i*2;
        double t_f_annomaly;
        switch (lambert_case) {
        case 0: t_f_annomaly = (α - sin(α) - β + sin(β)); break;
        case 1: t_f_annomaly = (2*PI - (α - sin(α)) - β + sin(β)); break;
        case 2: t_f_annomaly = (α - sin(α) + β - sin(β)); break;
        case 3: t_f_annomaly = (2*PI - (α - sin(α)) + β - sin(β)); break;
        }
        double t_f = sqrt(aa[i]*aa[i]*aa[i] / mu) * t_f_annomaly;
        ASSERT_ALOMST_EQUAL(t_f, (t2 - t1))

        /*double x = sqrt(a_min / aa[i]);
        double t_f_2 = (
            first_solution[i] ? 
            α - sin(α) - β + sin(β) :
            2*PI - α + sin(α) - β + sin(β)
        ) / (x*x*x) / sqrt(mu / (a_min*a_min*a_min));

        double diff_real = fabs(t_f - (t2 - t1)) / t_f;
        double y_calc = _lambert(sqrt(a_min / aa[i]), K, lambert_case);
        double diff_normalized = fabs(y_calc - y) / y_calc;
        DEBUG_SHOW_F(diff_real)
        DEBUG_SHOW_F(diff_normalized)
        DEBUG_SHOW_F(t_f)
        DEBUG_SHOW_F(t_f_2)
        DEBUG_SHOW_F(y_calc / sqrt(mu / (a_min*a_min*a_min)))*/
    }

    for (int i=0; i < tp->num_solutions; i++) {
        double r1_r2_outer_prod = pos1.cartesian.x * pos2.cartesian.y - pos1.cartesian.y * pos2.cartesian.x;
        // direct orbit is retrograde
        DEBUG_SHOW_F(r1_r2_outer_prod)
        DEBUG_SHOW_I(first_solution[i])
        bool direct_solution = (t2 - t1) < PI * sqrt(aa[i]*aa[i]*aa[i] / mu);
        bool is_prograde = direct_solution ^ (i == 1);
        tp->transfer_orbit[i] = OrbitFrom2PointsAndSMA(pos1, pos2, t1, aa[i], mu, is_prograde, i == 1);

        OrbitPos pos1_ = OrbitGetPosition(&tp->transfer_orbit[i], t1);
        OrbitPos pos2_ = OrbitGetPosition(&tp->transfer_orbit[i], t2);

        //ASSERT_ALOMST_EQUAL(pos1_.r, pos1.r)
        //ASSERT_ALOMST_EQUAL(pos2_.r, pos2.r)
    }

    // TODO: find dvs of solutions and rank them


    //OrbitPrint(&tp->transfer_orbit);
    //printf("\n");
}

void TransferPlanUpdate(TransferPlan* tp) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        tp->from = NULL;
        tp->to = NULL;
    }

    if (tp->from != NULL && tp->to != NULL) {
        TransferPlanSolve(tp);
    }

    Vector2 mouse_pos = GetMousePosition();
    Vector2 mouse_pos_world = GetMousePositionInWorld(&GetGlobalState()->camera);

    double longuitude = atan2(mouse_pos_world.y, mouse_pos_world.x);

    switch (tp->state) {
    case TPUISTATE_NONE: {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tp->from != NULL && tp->to != NULL){
            if (Vector2DistanceSqr(tp->departure_handle_pos, mouse_pos) < 20*20) {
                tp->state = TPUISTATE_MOD_DEPARTURE;
            }
            if (Vector2DistanceSqr(tp->arrival_handle_pos, mouse_pos) < 20*20) {
                tp->state = TPUISTATE_MOD_ARRIVAL;
            }
        }
        break;
    }
    case TPUISTATE_MOD_DEPARTURE:{
        double θ = longuitude - tp->from->orbit.lop;
        tp->departure_time = OrbitGetTimeUntilFocalAnomaly(&tp->from->orbit, θ, GetGlobalState()->time);
        break;
    }
    case TPUISTATE_MOD_ARRIVAL:{
        double θ = longuitude - tp->to->orbit.lop;
        tp->arrival_time = OrbitGetTimeUntilFocalAnomaly(&tp->to->orbit, θ, GetGlobalState()->time);
        break;
    }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
        tp->state = TPUISTATE_NONE;
    }
}

void TransferPlanDraw(TransferPlan* tp, const DrawCamera* cam) {
    if (tp->from == NULL || tp->to == NULL) {
        return;
    }
    Color colors[] = {
        RED,
        GREEN,
        BLUE,
        YELLOW
    };
    for (int i=0; i < tp->num_solutions; i++) {
        OrbitPos pos1 = OrbitGetPosition(&tp->transfer_orbit[i], tp->departure_time);
        OrbitPos pos2 = OrbitGetPosition(&tp->transfer_orbit[i], tp->arrival_time);
        SampleOrbitBounded(&tp->transfer_orbit[i], pos1, pos2, &tp->orbit_buffer[0], ORBIT_BUFFER_SIZE);
        //SampleOrbit(&tp->transfer_orbit[i], &tp->orbit_buffer[0], ORBIT_BUFFER_SIZE);
        CameraTransformBuffer(cam, &(tp->orbit_buffer)[0], ORBIT_BUFFER_SIZE);
        int solution = i*2 + !tp->transfer_orbit[i].prograde;
        DrawLineStrip(&tp->orbit_buffer[0], ORBIT_BUFFER_SIZE, colors[solution]);
    }

    tp->departure_handle_pos = CameraTransformV(cam, OrbitGetPosition(&tp->from->orbit, tp->departure_time).cartesian);
    tp->arrival_handle_pos = CameraTransformV(cam, OrbitGetPosition(&tp->to->orbit, tp->arrival_time).cartesian);

    DrawCircleV(tp->departure_handle_pos, 10, RED);
    DrawCircleV(tp->arrival_handle_pos, 10, RED);
}

void TransferPlanAddPlanet(TransferPlan* tp, const Planet* planet) {
    if (tp->state != TPUISTATE_NONE) {
        return;
    }
    if (tp->from == NULL) tp->from = planet;
    else {
        tp->to = planet;
        tp->departure_time = GetGlobalState()->time;
        double transfer_sma = (tp->from->orbit.sma + tp->to->orbit.sma) * .5;
        double hohmann_travel_time = tp->departure_time + sqrt(transfer_sma*transfer_sma*transfer_sma / planet->orbit.mu);
        tp->arrival_time = GetGlobalState()->time + hohmann_travel_time;
        TransferPlanSolve(tp);
    }
}
