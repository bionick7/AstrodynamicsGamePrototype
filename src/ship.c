#include "ship.h"
#include "utils.h"
#include "global_state.h"

void _ShipClicked(Ship* ship) {
    TransferPlanUISetShip(&(GlobalGetState()->active_transfer_plan), ship->id);
}

void ShipAssignTransfer(Ship* ship, TransferPlan tp) {
    double dv_tot = tp.dv1[tp.primary_solution] + tp.dv2[tp.primary_solution];
    if (dv_tot < ship->max_dv) {
        ship->next_plan = tp;
        ship->current_state = SHIP_STATE_PREPARE_TRANSFER;
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
}

void ShipDraw(Ship* ship, const DrawCamera* cam, int index_on_planet) {
    Vector2 draw_pos = CameraTransformV(cam, ship->position.cartesian);
    if (ship->current_state == SHIP_STATE_REST || ship->current_state == SHIP_STATE_PREPARE_TRANSFER) {
        double rad = fmax(CameraTransformS(cam, GetPlanet(ship->parent_planet)->radius), 4) + 8.0;
        double phase = 20.0 /  rad * index_on_planet;
        draw_pos = Vector2Add(FromPolar(rad, phase), draw_pos);
    }
    DrawRectangleV(draw_pos, (Vector2) {8, 8}, WHITE );

    float mouse_dist_sqr = Vector2DistanceSqr(GetMousePosition(), draw_pos);
    if (mouse_dist_sqr < 20*20) {
        // Hover
        DrawCircleLines(draw_pos.x, draw_pos.y, 10, RED);

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