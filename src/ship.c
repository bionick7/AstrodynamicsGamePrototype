#include "ship.h"
#include "utils.h"
#include "global_state.h"
#include "ui.h"

void _ShipClicked(Ship* ship) {
    if (GlobalGetState()->active_transfer_plan.ship == -1) {
        GetMainCamera()->paused = true;
        TransferPlanUISetShip(&(GlobalGetState()->active_transfer_plan), ship->id);
    }
}

bool ShipHasMouseHover(const Ship* ship, double* min_distance) {
    double dist = Vector2Distance(GetMousePosition(), ship->draw_pos);
    if (dist <= 10 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}

void ShipAssignTransfer(Ship* ship, TransferPlan tp) {
    printf("Assigning transfer plan %f to ship %s\n", tp.arrival_time, ship->name);
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
    if (dv_tot < ship->max_dv) {
        ship->next_plan = tp;
        ship->current_state = SHIP_STATE_PREPARE_TRANSFER;
    } else {
        printf("Not enough DV %f > %f\n", dv_tot, ship->max_dv);
    }
}

void ShipUpdate(Ship* ship) {
    time_type now = GlobalGetNow();
    TransferPlan tp = ship->next_plan;
    switch (ship->current_state) {
    case SHIP_STATE_REST: {
        ship->position = GetPlanet(ship->parent_planet)->position;
        break;
    }
    case SHIP_STATE_PREPARE_TRANSFER: {
        if (tp.departure_time <= now) {
            ship->parent_planet = -1;
            ship->current_state = SHIP_STATE_IN_TRANSFER;
            // Deliberate fall through
        } else {
            ship->position = GetPlanet(ship->parent_planet)->position;
            break;
        }
    }
    case SHIP_STATE_IN_TRANSFER: {
        if (tp.arrival_time <= now) {
            ship->parent_planet = tp.arrival_planet;
            ship->current_state = SHIP_STATE_REST;
            ship->position = GetPlanet(ship->parent_planet)->position;
        } else {
            ship->position = OrbitGetPosition(&tp.transfer_orbit[tp.primary_solution], now);
        }
        break;
    }
    }
    ship->draw_pos = CameraTransformV(GetMainCamera(), ship->position.cartesian);
    if (ship->current_state == SHIP_STATE_REST || ship->current_state == SHIP_STATE_PREPARE_TRANSFER) {
        double rad = fmax(CameraTransformS(GetMainCamera(), GetPlanet(ship->parent_planet)->radius), 4) + 8.0;
        double phase = 20.0 /  rad * ship->index_on_planet;
        ship->draw_pos = Vector2Add(FromPolar(rad, phase), ship->draw_pos);
    }
}

void ShipDraw(Ship* ship, const DrawCamera* cam) {
    SetRandomSeed(ship->id+1);
    Color color = (Color) {
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        GetRandomValue(0, 255),
        255
    };
    //printf("Drawing ship %s (%d, %d, %d)\n", ship->name, color.r, color.g, color.b);
    DrawRectangleV(Vector2SubtractValue(ship->draw_pos, 8/2), (Vector2) {8, 8}, color );

    if (ship->current_state == SHIP_STATE_PREPARE_TRANSFER || ship->current_state == SHIP_STATE_IN_TRANSFER) {
        OrbitPos to_departure = OrbitGetPosition(&ship->next_plan.transfer_orbit[ship->next_plan.primary_solution], 
            fmax(ship->next_plan.departure_time, GlobalGetNow())
        );
        OrbitPos to_arrival = OrbitGetPosition(&ship->next_plan.transfer_orbit[ship->next_plan.primary_solution], ship->next_plan.arrival_time);
        DrawOrbitBounded(&ship->next_plan.transfer_orbit[ship->next_plan.primary_solution], to_departure, to_arrival, color);
    }

    //float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), draw_pos);
    if (ship->mouse_hover) {
        // Hover
        DrawCircleLines(ship->draw_pos.x, ship->draw_pos.y, 10, RED);
        DrawTextEx(GetCustomDefaultFont(), ship->name, Vector2Add(ship->draw_pos, (Vector2){5, 5}), 16, 1, color);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _ShipClicked(ship);
        }
    }
}

void ShipInspect(const Ship* ship) {

    switch (ship->current_state) {
    case SHIP_STATE_REST:
    case SHIP_STATE_PREPARE_TRANSFER: {
        printf("%s : parked on %s, %f/%f m/s dv\n", ship->name, GetPlanet(ship->parent_planet)->name, ship->dv, ship->max_dv);
        break;
    }
    case SHIP_STATE_IN_TRANSFER: {
        printf("%s : in transfer[", ship->name);
        OrbitPrint(&ship->next_plan.transfer_orbit[ship->next_plan.primary_solution]);
        printf("] %f/%f m/s dv\n", ship->dv, ship->max_dv);
        break;
    }}
}