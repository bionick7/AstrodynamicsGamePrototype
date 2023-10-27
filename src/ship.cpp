#include "ship.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "ui.hpp"

void Ship::_OnClicked() {
    if (!IsIdValid(GlobalGetState()->active_transfer_plan.ship)) {
        GetMainCamera()->paused = true;
        TransferPlanUISetShip(&(GlobalGetState()->active_transfer_plan), id);
    }
}

void Ship::Make(const char *p_name) {
    strcpy(name, p_name);
    max_dv = 10000;
    v_e = 10000;
    max_capacity = 100000;
    respource_type = -1;
    respource_qtt = 0;

    /*color = (Color) {
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        255
    };*/
    color = PALETT_GREEN;
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

void Ship::AssignTransfer(TransferPlan tp) {
    printf("Assigning transfer plan %f to ship %s\n", tp.arrival_time, name);
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
    if (dv_tot < max_dv) {
        respource_type = 0;
        respource_qtt = GetPayloadCapacity(dv_tot);
        next_plan = tp;
        current_state = SHIP_STATE_PREPARE_TRANSFER;
    } else {
        printf("Not enough DV %f > %f\n", dv_tot, max_dv);
    }
}

void Ship::Update() {
    time_type now = GlobalGetNow();
    TransferPlan tp = next_plan;

    switch (current_state) {
    case SHIP_STATE_REST: {
        position = GetPlanet(parent_planet).position;
        break;
    }
    case SHIP_STATE_PREPARE_TRANSFER: {
        if (tp.departure_time <= now) {

            respource_type = 0;
            respource_qtt = GetPlanet(tp.departure_planet).DrawResource(0, GetPayloadCapacity(tp.tot_dv));

            char date_buffer[30];
            ForamtTime(date_buffer, 30, tp.departure_time);
            printf(":: On %s, \"%s\" picked up %f kg of %s on %s\n", 
                date_buffer,
                name,
                respource_qtt,
                resources_names[respource_type],
                GetPlanet(parent_planet).name
            );

            parent_planet = GetInvalidId();
            current_state = SHIP_STATE_IN_TRANSFER;

            // Deliberate fall through
        } else {
            position = GetPlanet(parent_planet).position;
            break;
        }
    }
    case SHIP_STATE_IN_TRANSFER: {
        if (tp.arrival_time <= now) {
            parent_planet = tp.arrival_planet;
            current_state = SHIP_STATE_REST;
            position = GetPlanet(parent_planet).position;

            resource_count_t delivered = GetPlanet(tp.arrival_planet).GiveResource(respource_type, respource_qtt);  // Ignore how much actually arrives (for now)

            char date_buffer[30];
            ForamtTime(date_buffer, 30, tp.arrival_time);
            printf(":: On %s, \"%s\" delivered %f kg of %s to %s\n", 
                date_buffer,
                name,
                delivered,
                resources_names[respource_type],
                GetPlanet(parent_planet).name
            );

            respource_qtt = 0;
            respource_type = -1;

        } else {
            position = OrbitGetPosition(&tp.transfer_orbit[tp.primary_solution], now);
        }
        break;
    }
    }
    draw_pos = CameraTransformV(GetMainCamera(), position.cartesian);
    if (current_state == SHIP_STATE_REST || current_state == SHIP_STATE_PREPARE_TRANSFER) {
        double rad = fmax(CameraTransformS(GetMainCamera(), GetPlanet(parent_planet).radius), 4) + 8.0;
        double phase = 20.0 /  rad * index_on_planet;
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
    }
}

void Ship::Draw(const DrawCamera* cam) const {
    //printf("Drawing ship %s (%d, %d, %d)\n", name, color.r, color.g, color.b);
    //printf("draw_pos: %f, %f\n", draw_pos.x, draw_pos.y);
    DrawRectangleV(Vector2SubtractValue(draw_pos, 4.0f), (Vector2) {8, 8}, color);

    if (current_state == SHIP_STATE_PREPARE_TRANSFER || current_state == SHIP_STATE_IN_TRANSFER) {
        OrbitPos to_departure = OrbitGetPosition(&next_plan.transfer_orbit[next_plan.primary_solution], 
            fmax(next_plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = OrbitGetPosition(&next_plan.transfer_orbit[next_plan.primary_solution], next_plan.arrival_time);
        DrawOrbitBounded(&next_plan.transfer_orbit[next_plan.primary_solution], to_departure, to_arrival, 0, color);
    }

}

void Ship::DrawUI(const DrawCamera* cam) {
    
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
        TextBox tb = TextBoxMake(
            GetScreenWidth() - 20*16 - 5, 5 + 200,
            20*16, GetScreenHeight() - 200 - 2*5, 
            16, MAIN_UI_COLOR
        );

        TextBoxEnclose(&tb, 2, 2, BG_COLOR, color);
        TextBoxWriteLine(&tb, name);

        char max_cargo_str[40];
        sprintf(max_cargo_str, "Cargo Cap. %.0f t", max_capacity);
        TextBoxWriteLine(&tb, max_cargo_str);

        char specific_impulse_str[40];
        sprintf(specific_impulse_str, "I_sp %.0f m/s", v_e);
        TextBoxWriteLine(&tb, specific_impulse_str);

        char maxdv_str[40];
        sprintf(maxdv_str, "dv %.0f m/s", max_dv);
        TextBoxWriteLine(&tb, maxdv_str);
    }
}

void Ship::Inspect() {
    switch (current_state) {
    case SHIP_STATE_REST:
    case SHIP_STATE_PREPARE_TRANSFER: {
        printf("%s : parked on %s, %f m/s dv\n", name, GetPlanet(parent_planet).name, max_dv);
        break;
    }
    case SHIP_STATE_IN_TRANSFER: {
        printf("%s : in transfer[", name);
        OrbitPrint(&next_plan.transfer_orbit[next_plan.primary_solution]);
        printf("] %f m/s dv\n", max_dv);
        break;
    }}
}