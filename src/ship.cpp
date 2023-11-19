#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "constants.hpp"

void Ship::_OnClicked() {
    if (GlobalGetState()->focused_ship == id) {
        GetScreenTransform()->focus = position.cartesian;
    } else {
        GlobalGetState()->focused_ship = id;
    }
}

void Ship::_OnNewPlanClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (tp_ui.IsActive() || IsIdValid(tp_ui.ship)) {
        return;
    }

    GetCalendar()->paused = true;

    // Append new plan

    ASSERT(prepared_plans_count != 0 || is_parked)
    if (prepared_plans_count >= SHIP_MAX_PREPARED_PLANS) {
        ERROR("Maximum transfer plan stack reached (ship %s)", name);
        return;
    }

    prepared_plans[prepared_plans_count] = TransferPlan();
    plan_edit_index = prepared_plans_count;

    Time min_time = 0;
    if (plan_edit_index == 0) {
        prepared_plans[plan_edit_index].departure_planet = parent_planet;
        min_time = GlobalGetNow();
    } else {
        prepared_plans[plan_edit_index].departure_planet = prepared_plans[plan_edit_index - 1].arrival_planet;
        min_time = prepared_plans[plan_edit_index - 1].arrival_time;
    }

    tp_ui.Make();
    tp_ui.SetPlan(&prepared_plans[plan_edit_index], id, min_time, 1e20);
    prepared_plans_count++;
}

void Ship::Load(const DataNode* data) {
    strcpy(name, data->Get("name", "UNNAMED"));
    is_parked = true;
    prepared_plans_count = 0;
    plan_edit_index = -1;
    
    max_capacity = data->GetF("capacity", 0) * 1000;  // t -> kg
    max_dv = data->GetF("dv", 0) * 1000;  // km/s -> m/s
    v_e = data->GetF("Isp", 0) * 1000;  // km/s -> m/s
    /*color = (Color) {
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        255
    };*/
    color = PALETTE_GREEN;
}

double Ship::GetPayloadCapacity(double dv) const {
    // dv = v_e * ln(1 + m_fuel/m_dry)
    // fuel_ratio = (exp(dv/v_e) - 1) / (exp(dv_max/v_e) - 1)
    // MOTM * fuel_ratio - OEM

    // v_e
    // dv_max
    // m_fuel_max = cargo_cap
    // OEM = cargo_cap/(exp(dv_max/v_e) -1)
    //double oem = max_capacity/max_fuel_ratio;
    double fuel_ratio = (exp(dv/v_e) - 1) / (exp(max_dv/v_e) - 1);
    return (1 - fuel_ratio) * max_capacity;
}

double Ship::GetFuelRequiredFull(double dv) const {
    // assuming max payload
    double fuel_ratio = (exp(dv/v_e) - 1) / (exp(max_dv/v_e) - 1);
    return fuel_ratio * max_capacity;
}

double Ship::GetFuelRequiredEmpty(double dv) const {
    // assuming no payload
    // dv = v_e * ln(1 + m_fuel/m_dry)
    // m_fuel = oem * (exp(dv/V_e) - 1)
    // max_capacity - oem = oem * (exp(dv_max/V_e) - 1)
    // oem = max_capacity / ((exp(dv_max/V_e) - 1) + 1)
    double oem = max_capacity / ((exp(max_dv/v_e) - 1) + 1);
    return oem * (exp(dv/v_e) - 1);
}


bool Ship::HasMouseHover(double* min_distance) const {
    double dist = Vector2Distance(GetMousePosition(), draw_pos);
    if (dist <= 10 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}
/*
TransferPlan* Ship::_NewTransferPlan() {
    ASSERT(prepared_plans_count != 0 || is_parked)
    if (prepared_plans_count >= SHIP_MAX_PREPARED_PLANS) {
        ERROR("Maximum transfer plan stack reached (ship %s)", name);
        return NULL;
    }
    prepared_plans[prepared_plans_count] = TransferPlan();

    if (prepared_plans_count == 0) {
        prepared_plans[prepared_plans_count].departure_planet = parent_planet;
    } else {
        prepared_plans[prepared_plans_count].departure_planet = prepared_plans[prepared_plans_count - 1].arrival_planet;
    }
    prepared_plans_count++;
    return GetEditedTransferPlan();
}*/

TransferPlan* Ship::GetEditedTransferPlan() {
    if (plan_edit_index < 0) return NULL;
    return &prepared_plans[plan_edit_index];
}

void Ship::ConfirmEditedTransferPlan() {
    ASSERT(prepared_plans_count != 0 || is_parked)
    ASSERT(!is_parked || IsIdValid(parent_planet))
    TransferPlan& tp = prepared_plans[prepared_plans_count - 1];
    if (prepared_plans_count == 1 && parent_planet != tp.departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at current planet)", name);
        return;
    }
    else if (prepared_plans_count > 1 && prepared_plans[prepared_plans_count - 2].arrival_planet != tp.departure_planet) {
        ERROR("Inconsistent transfer plan pushed for ship %s (does not start at planet last visited)", name);
        return;
    }
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
    if (dv_tot > max_dv) {
        ERROR("Not enough DV %f > %f", dv_tot, max_dv);
        return;
    }
    INFO("Assigning transfer plan %f to ship %s", tp.arrival_time, name);
    plan_edit_index = -1;
}

void Ship::CloseEditedTransferPlan() {
    PopTransferPlan(prepared_plans_count - 1);
}

void Ship::PopTransferPlan(int index) {
    // Does not call _EnsureContinuity to prevent invinite recursion
    if (index < 0 || index >= prepared_plans_count) {
        ERROR("Tried to remove transfer plan at invalid index %d (ship %s)", index, name);
        return;
    }

    for (int i=index; i < prepared_plans_count - 1; i++) {
        prepared_plans[i] = prepared_plans[i+1];
    }
    prepared_plans_count--;
    if (index == plan_edit_index) {
        plan_edit_index = -1;
    }
}

void Ship::StartEditingPlan(int index) {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    Time min_time = 0;
    if (prepared_plans_count == 0) {
        prepared_plans[prepared_plans_count].departure_planet = parent_planet;
        min_time = GlobalGetNow();
    } else {
        prepared_plans[prepared_plans_count].departure_planet = prepared_plans[prepared_plans_count - 1].arrival_planet;
        min_time = prepared_plans[prepared_plans_count - 1].arrival_time;
    }

    plan_edit_index = index;
    tp_ui.SetPlan(&prepared_plans[index], id, min_time, 1e20);
}

void Ship::Update() {
    Time now = GlobalGetNow();

    if (prepared_plans_count == 0 || (plan_edit_index == 0 && prepared_plans_count == 1)) {
        position = GetPlanet(parent_planet).position;
    } else {
        const TransferPlan& tp = prepared_plans[0];
        if (is_parked) {
            if (TimeIsEarlier(tp.departure_time, now)) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(parent_planet).position;
            }
        } else {
            if (TimeIsEarlier(tp.arrival_time, now)) {
                _OnArrival(tp);
            } else {
                position = OrbitGetPosition(&tp.transfer_orbit[tp.primary_solution], now);
            }
        }
    }

    draw_pos = GetScreenTransform()->TransformV(position.cartesian);
    if (is_parked) {
        double rad = fmax(GetScreenTransform()->TransformS(GetPlanet(parent_planet).radius), 4) + 8.0;
        double phase = 20.0 /  rad * index_on_planet;
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
    }
}

void _DrawShipAt(Vector2 pos, Color color) {
    DrawRectangleV(Vector2SubtractValue(pos, 4.0f), {8, 8}, color);
}

void Ship::Draw(const CoordinateTransform* c_transf) const {
    //printf("Drawing ship %s (%d, %d, %d)\n", name, color.r, color.g, color.b);
    //printf("draw_pos: %f, %f\n", draw_pos.x, draw_pos.y);
    _DrawShipAt(draw_pos, color);

    for (int i=0; i < prepared_plans_count; i++) {
        if (i == plan_edit_index) {
            continue;
        }
        const TransferPlan& plan = prepared_plans[i];
        OrbitPos to_departure = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            TimeLatest(plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            plan.arrival_time
        );
        DrawOrbitBounded(&plan.transfer_orbit[plan.primary_solution], to_departure, to_arrival, 0, 
            ColorAlpha(color, i == highlighted_plan_index ? 1 : 0.5)
        );
        if (i == plan_edit_index){
            DrawOrbitBounded(&plan.transfer_orbit[plan.primary_solution], to_departure, to_arrival, 0, MAIN_UI_COLOR);
        }
    }
    if (plan_edit_index >= 0 && IsIdValid(prepared_plans[plan_edit_index].arrival_planet)) {
        const TransferPlan& last_tp = prepared_plans[plan_edit_index];
        OrbitPos last_pos = OrbitGetPosition(
            &GetPlanet(last_tp.arrival_planet).orbit, 
            last_tp.arrival_time
        );
        Vector2 last_draw_pos = c_transf->TransformV(last_pos.cartesian);
        _DrawShipAt(last_draw_pos, ColorAlpha(color, 0.5));
    }
}

void Ship::DrawUI(const CoordinateTransform* c_transf) {
    //float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), draw_pos);
    if (mouse_hover) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, RED);
        DrawTextEx(GetCustomDefaultFont(), name, Vector2Add(draw_pos, {5, 5}), 16, 1, color);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }

    highlighted_plan_index = -1;
    if (mouse_hover || GlobalGetState()->active_transfer_plan.ship == id || GlobalGetState()->focused_ship == id) {
        int text_size = 16;
        UIContextCreate(
            GetScreenWidth() - 20*text_size - 5, 5 + 200,
            20*text_size, GetScreenHeight() - 200 - 2*5, 
            text_size, MAIN_UI_COLOR
        );

        UIContextEnclose(2, 2, BG_COLOR, color);
        UIContextWrite(name);
        
        char max_cargo_str[40];
        sprintf(max_cargo_str, "Cargo Cap. %.0f t", max_capacity);
        UIContextWrite(max_cargo_str);

        char specific_impulse_str[40];
        sprintf(specific_impulse_str, "I_sp %.0f m/s", v_e);
        UIContextWrite(specific_impulse_str);

        char maxdv_str[40];
        sprintf(maxdv_str, "dv %.0f m/s", max_dv);
        UIContextWrite(maxdv_str);

        Time now = GlobalGetNow();
        for (int i=0; i < prepared_plans_count; i++) {
            char tp_str[2][40];
            const char* resource_name;
            const char* departure_planet_name;
            const char* arrival_planet_name;

            if (prepared_plans[i].resource_transfer.resource_id < 0){
                resource_name = "EMPTY";
            } else {
                resource_name = resources_names[prepared_plans[i].resource_transfer.resource_id];
            }
            if (IsIdValid(prepared_plans[i].departure_planet)) {
                departure_planet_name = GetPlanet(prepared_plans[i].departure_planet).name;
            } else {
                departure_planet_name = "NOT SET";
            }
            if (IsIdValid(prepared_plans[i].arrival_planet)) {
                arrival_planet_name = GetPlanet(prepared_plans[i].arrival_planet).name;
            } else {
                arrival_planet_name = "NOT SET";
            }

            snprintf(tp_str[0], 40, "- %s (%3d D %2d H)",
                resource_name,
                (int) TimeDays(TimeSub(prepared_plans[i].arrival_time, now)),
                ((int) TimeSeconds(TimeSub(prepared_plans[i].arrival_time, now)) % 86400) / 3600
            );

            snprintf(tp_str[1], 40, "  %s >> %s", departure_planet_name, arrival_planet_name);

            // Double Button
            UIContextPushInset(2, UIContextCurrent().GetLineHeight() * 2);
            UIContextPushHSplit(0, -32);
            UIContextEnclose(0, 0, BG_COLOR, PALETTE_BLUE);
            UIContextWrite(tp_str[0]);
            UIContextWrite(tp_str[1]);
            ButtonStateFlags button_results = UIContextAsButton();
            if (button_results & BUTTON_STATE_FLAG_HOVER) {
                highlighted_plan_index = i;
            }
            if (button_results & BUTTON_STATE_FLAG_JUST_PRESSED) {
                StartEditingPlan(i);
            }
            UIContextPop();
            UIContextPushHSplit(-32, -1);
            //UIContextCurrent().text_size = text_size*2;
            UIContextWrite("X");
            UIContextEnclose(0, 0, BG_COLOR, PALETTE_BLUE);
            if (UIContextAsButton() & BUTTON_STATE_FLAG_JUST_PRESSED) {
                PopTransferPlan(i);
            }
            UIContextPop();
            UIContextPop();
        }
        if (UIContextDirectButton("+", 10) & BUTTON_STATE_FLAG_JUST_PRESSED) {
            _OnNewPlanClicked();
        }
    }
}

void Ship::Inspect() {
    if (is_parked) {
        INFO("%s : parked on %s, %f m/s d", name, GetPlanet(parent_planet).name, max_dv);
    } else {
        INFO("%s : in transfer[", name);
        //OrbitPrint(&next_plan.transfer_orbit[next_plan.primary_solution]);
        INFO("] %f m/s dv", max_dv);
    }
}

void Ship::_OnDeparture(const TransferPlan& tp) {
    respource_type = tp.resource_transfer.resource_id;
    respource_qtt = GetPlanet(tp.departure_planet).DrawResource(tp.resource_transfer.resource_id, tp.resource_transfer.quantity);

    GetPlanet(tp.departure_planet).DrawResource(RESOURCE_WATER, tp.fuel_mass);

    char date_buffer[30];
    FormatTime(date_buffer, 30, tp.departure_time);
    PLAYER_INFO(":: On %s, \"%s\" picked up %f kg of %s on %s", 
        date_buffer,
        name,
        respource_qtt,
        resources_names[respource_type],
        GetPlanet(parent_planet).name
    );

    parent_planet = GetInvalidId();
    is_parked = false;

    Update();
}

void Ship::_OnArrival(const TransferPlan& tp) {
    parent_planet = tp.arrival_planet;
    is_parked = true;
    position = GetPlanet(parent_planet).position;

    resource_count_t delivered = GetPlanet(tp.arrival_planet).GiveResource(respource_type, respource_qtt);  // Ignore how much actually arrives (for now)

    char date_buffer[30];
    FormatTime(date_buffer, 30, tp.arrival_time);
    PLAYER_INFO(":: On %s, \"%s\" delivered %f kg of %s to %s", 
        date_buffer,
        name,
        delivered,
        resources_names[respource_type],
        GetPlanet(parent_planet).name
    );
    PopTransferPlan(0);
    Update();
}

void Ship::_EnsureContinuity() {
    INFO("Ensure Continuity Call");
    if (prepared_plans_count == 0) return;
    entity_id_t planet_tracker = parent_planet;
    int start_index = 0;
    if (!is_parked) {
        planet_tracker = prepared_plans[0].arrival_planet;
        start_index = 1;
    }
    for(int i=start_index; i < prepared_plans_count; i++) {
        if (prepared_plans[i].departure_planet == planet_tracker) {
            planet_tracker = prepared_plans[i].arrival_planet;
        } else {
            PopTransferPlan(i);
        }
    }
}