#include "transfer_plan.hpp"
#include "planet.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "ui.hpp"

#include <time.h>

double _lambert(double x, double K, int solution) {
    // x² = a_min/a
    // y = t_f * sqrt(mu / (a_min*a_min*a_min))
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

double _lambert_derivative(double x, double K, int solution) {
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

double _solve_lambert_between_bounds(double y, double K, double xl, double xr, int solution) {
    double yl = _lambert(xl, K, solution) - y;
    double yr = _lambert(xr, K, solution) - y;
    if (yl*yr > 0.0) {FAIL_FORMAT("yl = %f and yr = %f have the same sign (y = %f, xl = %f, xr = %f, K = %f, sol = %d\n)", yl, yr, y, xl, xr, K, solution);}
    //ASSERT(yl*yr < 0.0)
    int counter = 0;
    double xm, ym;
    do {
        xm = (xr + xl) / 2.0;
        ym = _lambert(xm, K, solution) - y;
        if (ym * yr < 0.0) xl = xm;
        else xr = xm;
        if (counter++ > 1000) FAIL("Counter exceeded")
    } while (fabs(ym) > 1e-6 * y);
    
    double test_y = _lambert(xm, K, solution);
    ASSERT_ALOMST_EQUAL(test_y, y)
    return xm;
}

double _solve_lambert_by_newton(double y, double K, int solution) {
    double xs;
    switch (solution) {
    case 4: xs = 2 * (K*K - 1) / y; break;
    case 5: xs = -2 * (K*K + 1) / y; break;
    default: FAIL("newtown's method only implemented for cases 4 and 5")
    }

    int counter = 0;
    double ys, dydx_s;
    do {
        ys = _lambert(xs, K, solution) - y;
        dydx_s = _lambert_derivative(xs, K, solution);
        xs -= ys / dydx_s;
        if (counter++ > 1000) FAIL_FORMAT("Counter exceeded y=%f K=%f, solution=%d, x settled at %f with dx %f ", y, K, solution, xs, ys / dydx_s)
    } while (fabs(ys) > 1e-6);
    
    double test_y = _lambert(xs, K, solution);
    ASSERT_ALOMST_EQUAL(test_y, y)
    return xs;
}

void TransferPlanSolve(TransferPlan* tp) {
    const Planet& from = GetPlanet(tp->departure_planet);
    const Planet& to = GetPlanet(tp->arrival_planet);
    ASSERT_ALOMST_EQUAL(from.orbit.mu, to.orbit.mu)
    double mu = from.orbit.mu;

    double t1 = tp->departure_time;
    double t2 = tp->arrival_time;
    OrbitPos pos1 = OrbitGetPosition(&from.orbit, t1);
    OrbitPos pos2 = OrbitGetPosition(&to.orbit, t2);
    
    double c = Vector2Distance(pos1.cartesian, pos2.cartesian);
    double r_sum = pos1.r + pos2.r;
    double a_min = 0.25 * (r_sum + c);
    double K = sqrt((r_sum - c) / (r_sum + c));
    double y = (t2 - t1) * sqrt(mu / (a_min*a_min*a_min));

    bool is_ellipse[2] =  {y > _lambert(1e-3, K, 0), y > _lambert(1e-3, K, 2)};
    bool first_solution[2] = { y < _lambert(1, K, 0), y < _lambert(1, K, 2) };

    double aa[2];

    tp->num_solutions = 2;
    for (int i=0; i < 2; i++) {
        if (is_ellipse[i]) {
            int lambert_case = (first_solution[i] ? 0 : 1) + i*2;
            double x = _solve_lambert_between_bounds(y, K, 1e-5, 1.0, lambert_case);
            aa[i] = a_min / (x*x);
        } else {
            int lambert_case = i + 4;
            double x;
            if (y > _lambert(-2, K, i + 4)) {
                x = _solve_lambert_between_bounds(y, K, -2, 1e-5, lambert_case);
            } else {
                x = _solve_lambert_by_newton(y, K, lambert_case);
            }
            aa[i] = -a_min / (x*x);
        }
    }

    // Verify (for ellipses)

    for (int i=0; i < tp->num_solutions; i++) {
        if (!is_ellipse[i]) continue;
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
    }

    for (int i=0; i < tp->num_solutions; i++) {
        //double r1_r2_outer_prod = Determinant(pos1.cartesian, pos2.cartesian);
        // Direct orbit is retrograde
        //DEBUG_SHOW_F(r1_r2_outer_prod)
        //DEBUG_SHOW_I(first_solution[i])
        bool is_prograde;
        if (is_ellipse[i]) {
            bool direct_solution = (t2 - t1) < PI * sqrt(fabs(aa[i])*aa[i]*aa[i] / mu);
            is_prograde = direct_solution ^ (i == 1);
        } else {
            is_prograde = i == 1;
        }
        tp->transfer_orbit[i] = OrbitFrom2PointsAndSMA(pos1, pos2, t1, aa[i], mu, is_prograde, i == 1);
        //OrbitPrint(&tp->transfer_orbit[i]); printf("\n");

        OrbitPos pos1_tf = OrbitGetPosition(&tp->transfer_orbit[i], t1);
        OrbitPos pos2_tf = OrbitGetPosition(&tp->transfer_orbit[i], t2);

        //SHOW_V2(OrbitGetVelocity(&tp->transfer_orbit[i], pos1_tf))
        //SHOW_V2(OrbitGetVelocity(&tp->from->orbit, pos1))
        tp->departure_dvs[i] = Vector2Subtract(OrbitGetVelocity(&tp->transfer_orbit[i], pos1_tf), OrbitGetVelocity(&from.orbit, pos1));
        tp->arrival_dvs[i] = Vector2Subtract(OrbitGetVelocity(&to.orbit, pos2), OrbitGetVelocity(&tp->transfer_orbit[i], pos2_tf));

        tp->dv1[i] = from.GetDVFromExcessVelocity(tp->departure_dvs[i]);
        tp->dv2[i] = to.GetDVFromExcessVelocity(tp->arrival_dvs[i]);
        //ASSERT_ALOMST_EQUAL(pos1_.r, pos1.r)
        //ASSERT_ALOMST_EQUAL(pos2_.r, pos2.r)
    }

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

void TransferPlanUIMake(TransferPlanUI* ui) {
    ui->plan.departure_planet = GetInvalidId();
    ui->plan.arrival_planet = GetInvalidId();
    ui->ship = GetInvalidId();
    ui->plan.num_solutions = 0;
    ui->plan.resource_transfer = EMPTY_TRANSFER;
    ui->is_dragging_departure = false;
    ui->is_dragging_arrival = false;
    ui->departure_handle_pos = (Vector2) {0};
    ui->arrival_handle_pos = (Vector2) {0};
    ui->redraw_queued = false;
}

void TransferPlanUIUpdate(TransferPlanUI* ui) {
    if (!TransferPlanUIIsActive(ui)) {
        return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        TransferPlanUIMake(ui);
        return;
    }

    Ship& ship = GetShip(ui->ship);

    if (IsIdValid(ui->plan.departure_planet) && IsIdValid(ui->plan.arrival_planet) && ui->redraw_queued) {
        TransferPlanSolve(&ui->plan);
        ui->is_valid = ui->plan.num_solutions > 0 && ui->plan.tot_dv <= ship.max_dv;
        ui->is_valid = ui->is_valid && ui->plan.departure_time > GetTime();

        ui->redraw_queued = false;
    }

    if (ui->is_valid) {
        TransferPlanUISetPayloadMass(ui, ship.GetPayloadCapacity(ui->plan.tot_dv));
    } else {
        TransferPlanUISetPayloadMass(ui, 0);
    }

    if (IsKeyPressed(KEY_ENTER) && ui->is_valid) {
        ship.AssignTransfer(ui->plan);
        TransferPlanUIMake(ui);
    }
}

void _TransferPlanInitialize(TransferPlan* tp, time_type now) {
    // Sets departure and arrival time to the dv-cheapest values (assuming hohmann transfer)
    ASSERT(IsIdValid(tp->departure_planet))
    ASSERT(IsIdValid(tp->arrival_planet))
    const Planet& from = GetPlanet(tp->departure_planet);
    const Planet& to = GetPlanet(tp->arrival_planet);
    double mu = from.orbit.mu;
    double hohmann_a = (from.orbit.sma + to.orbit.sma) * 0.5;
    double hohmann_flight_time = sqrt(hohmann_a*hohmann_a*hohmann_a / mu) * PI;
    double p1_mean_motion = OrbitGetMeanMotion(&from.orbit);
    double p2_mean_motion = OrbitGetMeanMotion(&to.orbit);
    double relative_mean_motion = p2_mean_motion - p1_mean_motion;
    double current_relative_annomaly = OrbitGetPosition(&to.orbit, now).longuitude - OrbitGetPosition(&from.orbit, now).longuitude;
    double target_relative_anomaly = PosMod(PI - p2_mean_motion * hohmann_flight_time, 2*PI);
    double departure_wait_time = (target_relative_anomaly - current_relative_annomaly) / relative_mean_motion;
    double relative_period = fabs(2 * PI / relative_mean_motion);
    departure_wait_time = PosMod(departure_wait_time, relative_period);
    tp->departure_time = now + departure_wait_time;
    tp->arrival_time = now + departure_wait_time + hohmann_flight_time;
}

void _DrawSweep(const Orbit* orbit, time_t from, time_t to, Color color) {
    OrbitPos from_pos = OrbitGetPosition(orbit, from);
    OrbitPos to_pos = OrbitGetPosition(orbit, to);

    int full_orbits = floor((to - from) / OrbitGetPeriod(orbit));
    double offset_per_pixel = CameraInvTransformS(GetMainCamera(), 1);
    for (int i=1; i <= full_orbits; i++) {
        DrawOrbitWithOffset(orbit, offset_per_pixel * -3 * i, color);
    }
    DrawOrbitBounded(orbit, from_pos, to_pos, offset_per_pixel*3, color);
}

void _DrawTransferOrbit(TransferPlanUI* ui, int solution, bool is_secondary) {
    TransferPlan* tp = &ui->plan;
    const Planet& from = GetPlanet(tp->departure_planet);
    const Planet& to = GetPlanet(tp->arrival_planet);
    Color velocity_color = YELLOW;
    Color orbit_color = TRANSFER_UI_COLOR;
    if (is_secondary) {
        velocity_color = ColorTint(velocity_color, GRAY);
        orbit_color = ColorTint(orbit_color, GRAY);
    }
    time_type now = GlobalGetState()->time;
    OrbitPos pos1 = OrbitGetPosition(&tp->transfer_orbit[solution], tp->departure_time);
    OrbitPos pos2 = OrbitGetPosition(&tp->transfer_orbit[solution], tp->arrival_time);
    _DrawSweep(&from.orbit, now, tp->departure_time, orbit_color);
    _DrawSweep(&to.orbit,   now, tp->arrival_time,   orbit_color);
    DrawLineV(
        ui->departure_handle_pos,
        Vector2Add(ui->departure_handle_pos, Vector2Scale(tp->departure_dvs[solution], 0.01)),
        velocity_color
    );
    DrawLineV(
        ui->arrival_handle_pos,
        Vector2Add(ui->arrival_handle_pos, Vector2Scale(tp->arrival_dvs[solution], 0.01)),
        velocity_color
    );
    DrawOrbitBounded(&tp->transfer_orbit[solution], pos1, pos2, 0, orbit_color);
}


time_type _DrawHandle(const DrawCamera* cam, Vector2 pos, const Orbit* orbit, time_type current, bool* is_dragging) {
    Vector2 radial_dir = Vector2Normalize(CameraInvTransformV(cam, pos));
    radial_dir.y = -radial_dir.y;
    Vector2 tangent_dir = Vector2Rotate(radial_dir, PI/2);

    Vector2 node_pos = Vector2Add(pos, Vector2Scale(radial_dir, 20));
    Vector2 plus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, -23));
    Vector2 minus_pos = Vector2Add(node_pos, Vector2Scale(tangent_dir, 23));
    time_type period = OrbitGetPeriod(orbit);
    if (DrawTriangleButton(pos, Vector2Scale(radial_dir, 20), 10, TRANSFER_UI_COLOR) & BUTTON_STATE_FLAG_JUST_PRESSED) {
        *is_dragging = true;
    }
    if (DrawCircleButton(plus_pos, 10, TRANSFER_UI_COLOR) & BUTTON_STATE_FLAG_JUST_PRESSED) {
        current += period;
    }
    if (DrawCircleButton(minus_pos, 10, TRANSFER_UI_COLOR) & BUTTON_STATE_FLAG_JUST_PRESSED) {
        current -= period;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        *is_dragging = false;
    }
    if (*is_dragging) {
        time_type now = GlobalGetNow();
        int full_orbits = floor((current - now) / period);
        time_type t0 = now + full_orbits * period;

        Vector2 mouse_pos_world = GetMousePositionInWorld();
        double longuitude = atan2(mouse_pos_world.y, mouse_pos_world.x);
        double θ = longuitude - orbit->lop;
        current = t0 + OrbitGetTimeUntilFocalAnomaly(orbit, θ, t0);
        ASSERT(current > now);
    }
    return current;
}

TextBox textbox;

void _TransferPlanUIDrawText(const TransferPlan* tp, const Ship& ship) {
    TextBoxEnclose(&textbox, 2, 2, BG_COLOR, TRANSFER_UI_COLOR);
    char departure_time_outpstr[30];
    char arrival_time_outpstr[30];
    char departure_time_str[40] = "Departs in ";
    char arrival_time_str[40] =   "Arrives in ";
    char dv1_str[40];
    char dv2_str[40];
    char dvtot_str[40];
    char payload_str[40];

    double total_dv = tp->dv1[tp->primary_solution] + tp->dv2[tp->primary_solution];
    ForamtTime(departure_time_outpstr, 30, tp->departure_time - GlobalGetNow());
    ForamtTime(arrival_time_outpstr, 30, tp->arrival_time - GlobalGetNow());
    sprintf(dv1_str,   "DV 1      %5.3f km/s", tp->dv1[tp->primary_solution]/1000.0);
    sprintf(dv2_str,   "DV 2      %5.3f km/s", tp->dv2[tp->primary_solution]/1000.0);
    sprintf(dvtot_str, "DV Tot    %5.3f km/s", total_dv/1000.0);
    double capacity = ship.GetPayloadCapacity(total_dv);
    sprintf(payload_str, "Payload cap.  %3.0f %% (%.0f / %.0f t)", 
        capacity / ship.max_capacity * 100,
        capacity / 1000.0,
        ship.max_capacity / 1000.0
    );

    TextBoxWriteLine(&textbox, strcat(departure_time_str, departure_time_outpstr));
    TextBoxWriteLine(&textbox, strcat(arrival_time_str, arrival_time_outpstr));
    TextBoxWriteLine(&textbox, "=====================");
    //TextBoxWriteLine(&textbox, dv1_str);
    //TextBoxWriteLine(&textbox, dv2_str);
    TextBoxWriteLine(&textbox, dvtot_str);
    TextBoxWriteLine(&textbox, "=====================");
    TextBoxWriteLine(&textbox, payload_str);
}

void TransferPlanUIDraw(TransferPlanUI* ui, const DrawCamera* cam) {
    TransferPlan* tp = &ui->plan;
    if (!TransferPlanUIIsActive(ui)) {
        return;
    }
    const Planet& from = GetPlanet(tp->departure_planet);
    const Planet& to = GetPlanet(tp->arrival_planet);
    const Ship& ship = GetShip(ui->ship);

    textbox = TextBoxMake(
        GetScreenWidth() - 20*16 - 5, 5 + 20,
        20*16, MinInt(200, GetScreenHeight()) - 2*5 - 20, 
        16, TRANSFER_UI_COLOR
    );

    ui->departure_handle_pos = CameraTransformV(cam, OrbitGetPosition(&from.orbit, tp->departure_time).cartesian);
    ui->arrival_handle_pos = CameraTransformV(cam, OrbitGetPosition(&to.orbit, tp->arrival_time).cartesian);

    time_type now = GlobalGetState()->time;
    time_t new_departure_time = _DrawHandle(cam, ui->departure_handle_pos, &from.orbit, tp->departure_time, &ui->is_dragging_departure);
    time_t new_arrival_time = _DrawHandle(cam, ui->arrival_handle_pos, &to.orbit, tp->arrival_time, &ui->is_dragging_arrival);
    if (new_departure_time >= now && new_departure_time < tp->arrival_time){
        tp->departure_time = new_departure_time;
        ui->redraw_queued = true;
    }
    if (new_arrival_time >= tp->departure_time){
        tp->arrival_time = new_arrival_time;
        ui->redraw_queued = true;
    }

    if (tp->num_solutions == 1) {
        _DrawTransferOrbit(ui, tp->primary_solution, false);
    } else if (tp->num_solutions == 2) {
        _DrawTransferOrbit(ui, tp->primary_solution, false);
        _DrawTransferOrbit(ui, 1 - tp->primary_solution, true);
    }
    if (ui->is_valid) {
        _TransferPlanUIDrawText(tp, ship);
    }
    else if (sin(GetTime() * 6.0) > 0.0) {
        char transfer_str[100];
        if (ui->plan.tot_dv > ship.max_dv) {
            sprintf(transfer_str, "INVALID TRANSFER: %5.3f > %5.3f km/s", 
                ui->plan.tot_dv / 1000,
                ship.max_dv / 1000
            );
        } else if (ui->plan.departure_time < now) {
            strcpy(transfer_str, "INVALID TRANSFER: Departuring in the past");
        }
        textbox.height = 30;
        TextBoxEnclose(&textbox, 2, 2, BG_COLOR, TRANSFER_UI_COLOR);
        TextBoxWriteLine(&textbox, transfer_str);
    }
}

void TransferPlanUISetShip(TransferPlanUI* ui, entity_id_t ship) {
    if (ui->is_dragging_departure || ui->is_dragging_arrival) {
        return;
    }
    ui->ship = ship;
    ui->plan.departure_planet = GetShip(ship).parent_planet;
    if (ui->plan.departure_planet != entt::null && ui->plan.arrival_planet != entt::null) {
        _TransferPlanInitialize(&ui->plan, GlobalGetState()->time);
    }
}


void TransferPlanUISetResourceType(TransferPlanUI* ui, int resource_type) {
    ui->plan.resource_transfer.resource_id = resource_type;
}

void TransferPlanUISetPayloadMass(TransferPlanUI* ui, resource_count_t payload) {
    ui->plan.resource_transfer.quantity = payload;
}

void TransferPlanUISetDestination(TransferPlanUI* ui, entity_id_t planet) {
    if (ui->is_dragging_departure || ui->is_dragging_arrival || !IsIdValid(ui->ship) || !IsIdValid(ui->plan.departure_planet)) {
        return;
    }
    ui->plan.arrival_planet = planet;
    if (ui->plan.departure_planet != entt::null && ui->plan.arrival_planet != entt::null) {
        _TransferPlanInitialize(&ui->plan, GlobalGetState()->time);
    }
}

bool TransferPlanUIIsActive(TransferPlanUI* ui) {
    if (!IsIdValid(ui->ship)) return false;
    if (!IsIdValid(ui->plan.arrival_planet)) return false;
    if (!IsIdValid(ui->plan.arrival_planet)) return false;
    return true;
}

int TransferPlanTests() {
    const double epsilon = 1e-5;
    for (double K = 0.0; K < 1.0; K += 0.2) {
        for(double x = -5.0; x < -0.1; x += 0.1) {
            for (int solution = 4; solution <= 5; solution++) {
                double ddx = _lambert_derivative(x, K, solution);
                double central_difference = (_lambert(x + epsilon, K, solution) - _lambert(x - epsilon, K, solution)) / (2*epsilon);
                if (fabs(ddx - central_difference) > epsilon) {
                    printf("Error for x=%f, K=%f, solution=%d d/dx expected to be %f, but is measured to be %f\n", x, K, solution, ddx, central_difference);
                    return 1;
                }
            }
        }
    }
    return 0;
}