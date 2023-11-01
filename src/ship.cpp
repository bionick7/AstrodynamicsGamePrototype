#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"

void Ship::_OnClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (!tp_ui.IsActive() && !IsIdValid(tp_ui.ship)) {
        GetScreenTransform()->paused = true;
        tp_ui.SetShip(id);
    }
}

void Ship::Make(const char *p_name) {
    strcpy(name, p_name);
    max_dv = 10000;
    v_e = 10000;
    max_capacity = 100000;
    is_parked = true;
    prepared_plans_count = 0;
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

bool Ship::HasMouseHover(double* min_distance) const {
    double dist = Vector2Distance(GetMousePosition(), draw_pos);
    if (dist <= 10 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}


TransferPlan* Ship::NewTransferPlan() {
    ASSERT(prepared_plans_count != 0 || is_parked)
    if (prepared_plans_count >= SHIP_MAX_PREPARED_PLANS) {
        printf("Maximum transfer plan stack reached (ship %s)\n", name);
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
}

TransferPlan* Ship::GetEditedTransferPlan() {
    return &prepared_plans[prepared_plans_count - 1];
}

void Ship::ConfirmEditedTransferPlan() {
    ASSERT(prepared_plans_count != 0 || is_parked)
    ASSERT(!is_parked || IsIdValid(parent_planet))
    TransferPlan& tp = prepared_plans[prepared_plans_count - 1];
    if (prepared_plans_count == 1 && parent_planet != tp.departure_planet) {
        printf("Inconsistent transfer plan pushed for ship %s (does not start at current planet)\n", name);
        return;
    }
    else if (prepared_plans_count > 1 && prepared_plans[prepared_plans_count - 2].arrival_planet != tp.departure_planet) {
        printf("Inconsistent transfer plan pushed for ship %s (does not start at planet last visited)\n", name);
        return;
    }
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
    if (dv_tot > max_dv) {
        printf("Not enough DV %f > %f\n", dv_tot, max_dv);
        return;
    }
    printf("Assigning transfer plan %f to ship %s\n", tp.arrival_time, name);
    confirmed_plans_count = prepared_plans_count;
}

void Ship::CloseEditedTransferPlan() {
    PopTransferPlan(prepared_plans_count - 1);
}

void Ship::PopTransferPlan(int index) {
    // Does not call _EnsureContinuity to prevent invinite recursion
    if (index < 0 || index >= prepared_plans_count) {
        printf("Tried to remove transfer plan at invalid index %d (ship %s)\n", index, name);
        return;
    }

    for (int i=index; i < prepared_plans_count - 1; i++) {
        prepared_plans[i] = prepared_plans[i+1];
    }
    prepared_plans_count--;
    if (index < confirmed_plans_count) {
        confirmed_plans_count--;
    }
}

void Ship::Update() {
    time_type now = GlobalGetNow();

    if (confirmed_plans_count == 0) {
        position = GetPlanet(parent_planet).position;
    } else {
        const TransferPlan& tp = prepared_plans[0];
        if (is_parked) {
            if (tp.departure_time <= now) {
                _OnDeparture(tp);
            } else {
                position = GetPlanet(parent_planet).position;
            }
        } else {
            if (tp.arrival_time <= now) {
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
    DrawRectangleV(Vector2SubtractValue(pos, 4.0f), (Vector2) {8, 8}, color);
}

void Ship::Draw(const CoordinateTransform* c_transf) const {
    //printf("Drawing ship %s (%d, %d, %d)\n", name, color.r, color.g, color.b);
    //printf("draw_pos: %f, %f\n", draw_pos.x, draw_pos.y);
    _DrawShipAt(draw_pos, color);

    for (int i=0; i < confirmed_plans_count; i++) {
        const TransferPlan& plan = prepared_plans[i];
        OrbitPos to_departure = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            fmax(plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = OrbitGetPosition(
            &plan.transfer_orbit[plan.primary_solution], 
            plan.arrival_time
        );
        DrawOrbitBounded(&plan.transfer_orbit[plan.primary_solution], to_departure, to_arrival, 0, ColorAlpha(color, i == 0 ? 1 : 0.5));
    }
    if (confirmed_plans_count > 0) {
        const TransferPlan& last_tp = prepared_plans[confirmed_plans_count- 1];
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
        DrawTextEx(GetCustomDefaultFont(), name, Vector2Add(draw_pos, (Vector2){5, 5}), 16, 1, color);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }
    if (mouse_hover || GlobalGetState()->active_transfer_plan.ship == id) {
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

        time_type now = GlobalGetNow();
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
                (int)(prepared_plans[i].arrival_time - now) / 86400,
                ((int)(prepared_plans[i].arrival_time - now) % 86400) / 3600
            );

            snprintf(tp_str[1], 40, "  %s >> %s", departure_planet_name, arrival_planet_name);

            UIContextPushInset(2, UIContextCurrent().GetLineHeight() * 2);
                UIContextPushHSplit(0, -32);
                    UIContextEnclose(0, 0, BG_COLOR, PALETTE_BLUE);
                    UIContextWrite(tp_str[0]);
                    UIContextWrite(tp_str[1]);
                    if (UIContextAsButton() & BUTTON_STATE_FLAG_JUST_PRESSED) {
                        // TBD: edit previous transfer
                        PopTransferPlan(i);
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
    }
}

void Ship::Inspect() {
    if (is_parked) {
        printf("%s : parked on %s, %f m/s dv\n", name, GetPlanet(parent_planet).name, max_dv);
    } else {
        printf("%s : in transfer[", name);
        //OrbitPrint(&next_plan.transfer_orbit[next_plan.primary_solution]);
        printf("] %f m/s dv\n", max_dv);
    }
}

void Ship::_OnDeparture(const TransferPlan& tp) {
    respource_type = tp.resource_transfer.resource_id;
    respource_qtt = GetPlanet(tp.departure_planet).DrawResource(tp.resource_transfer.resource_id, tp.resource_transfer.quantity);

    char date_buffer[30];
    FormatTime(date_buffer, 30, tp.departure_time);
    printf(":: On %s, \"%s\" picked up %f kg of %s on %s\n", 
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
    printf(":: On %s, \"%s\" delivered %f kg of %s to %s\n", 
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
    printf("Ensure Continuity\n");
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