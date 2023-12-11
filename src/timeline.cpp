#include "time.hpp"
#include "timeline.hpp"
#include "global_state.hpp"
#include "constants.hpp"
#include "debug_drawing.hpp"

struct TimeLineCoordinateData {
    int x0;
    int y0;
    int w;
    int h;

    int* planet_coords;
};

const int PIXELS_PER_DAY = 60;

int GetPlanetCoord(const TimeLineCoordinateData* tcd, entity_id_t planet) {
    return tcd->planet_coords[planet];
}

int GetTimeCoord(const TimeLineCoordinateData* tcd, timemath::Time t) {
    timemath::Time ref_time = GlobalGetNow();
    return tcd->y0 + 24 + (t - ref_time).Seconds() * PIXELS_PER_DAY / 86400;
}

timemath::Time GetEndTime(const TimeLineCoordinateData* tcd) {
    timemath::Time ref_time = GlobalGetNow();
    return ref_time + timemath::Time(((float)tcd->h - 24.f) / (float) PIXELS_PER_DAY * 86400);
}

void _DrawPlanets(TimeLineCoordinateData* tcd, const Planets* planets) {
    double min_sma = INFINITY, max_sma = 0;
    for (int planet_index = 0; planet_index < planets->planet_count; planet_index++) {
        const Planet* planet = &planets->planet_array[planet_index];
        if (planet->orbit.sma > max_sma) max_sma = planet->orbit.sma;
        if (planet->orbit.sma < min_sma) min_sma = planet->orbit.sma;
    }

    int min_planet_spacing = 100;
    if (min_planet_spacing * (planets->planet_count + 2) > tcd->w) {
        min_planet_spacing = tcd->w / (planets->planet_count + 2);
    }

    int previous_x = 0;
    for (int planet_index = 0; planet_index < planets->planet_count; planet_index++) {
        const Planet* planet = &planets->planet_array[planet_index];
        double ratio = log(planet->orbit.sma/min_sma) / log(max_sma/min_sma);
        int x = ratio * (tcd->w - min_planet_spacing) + tcd->x0;
        if (x - previous_x < min_planet_spacing) {
            x = previous_x + min_planet_spacing;
        }
        //DebugPrintText("%s: sma = %f, x = %d", planet->name, planet->orbit.sma, x);
        DrawTextAligned(planet->name, {(float)x, (float)tcd->y0 + 18}, TEXT_ALIGNMENT_HCENTER | TEXT_ALIGNMENT_BOTTOM, MAIN_UI_COLOR);
        DrawLine(x, tcd->y0 + 24, x, tcd->y0 + tcd->h, MAIN_UI_COLOR);
        previous_x = x;
        tcd->planet_coords[planet_index] = x;
    }
}

void  _QuestDrawLine(TimeLineCoordinateData* tcd, const Quest* q, bool active) {
    entity_id_t from = q->departure_planet;
    entity_id_t to = q->arrival_planet;
    timemath::Time pickup_time = q->pickup_expiration_time;
    timemath::Time delivery_time = q->delivery_expiration_time;

    Vector2 start_pos = {
        GetPlanetCoord(tcd, from),
        GetTimeCoord(tcd, pickup_time)
    };

    Vector2 end_pos = {
        GetPlanetCoord(tcd, to),
        GetTimeCoord(tcd, delivery_time)
    };

    Color c = MAIN_UI_COLOR;
    if (!active) {
        c = ColorAlphaBlend(BG_COLOR, c, GetColor(0x80808080u));
    }

    //Vector2 scaled_unit = Vector2Scale(Vector2Normalize(Vector2Subtract(end_pos, start_pos)), 10);
    Vector2 scaled_unit = {(end_pos.x - start_pos.x) > 0 ? 10 : -10, 0};
    start_pos = Vector2Add(start_pos, scaled_unit);
    end_pos = Vector2Subtract(end_pos, scaled_unit);
    DrawTriangle(
        end_pos, 
        Vector2Add(Vector2Subtract(end_pos, Vector2Scale(scaled_unit, 1.5)), Vector2Scale(Vector2Rotate(scaled_unit, -PI/2), .5)),
        Vector2Add(Vector2Subtract(end_pos, Vector2Scale(scaled_unit, 1.5)), Vector2Scale(Vector2Rotate(scaled_unit,  PI/2), .5)),
        c
    );

    Vector2 helper_start = Vector2Add(start_pos, Vector2Scale(scaled_unit, 4));
    Vector2 helper_end = Vector2Subtract(end_pos, Vector2Scale(scaled_unit, 4));

    DrawLineV(start_pos, helper_start, c);
    DrawLineV(helper_start, helper_end, c);
    DrawLineV(helper_end, end_pos, c);

    /*Vector2 start_control = start_pos;
    start_control.x = (start_pos.x * 2 + end_pos.x) / 3;
    Vector2 end_control = end_pos;
    end_control.x = (end_pos.x * 2 + start_pos.x) / 3;
    DrawLineBezierCubic(start_pos, end_pos,
        start_control, end_control,
        1, c
    );*/
}

void _DrawQuests(TimeLineCoordinateData* tcd, const QuestManager* qm) {
    for(int i=0; i < qm->GetAvailableQuests(); i++) {
        const Quest* q = &qm->available_quests[i];
        if (!q->IsValid()) {
            continue;
        }
        _QuestDrawLine(tcd, q, false);
    }
    for(auto it = qm->active_quests.GetIter(); it; it++) {
        _QuestDrawLine(tcd, qm->active_quests.Get(it), true);
    }
}

int _Assign_And_Ret(int* var, int value) {
    *var = value;
    return value;
}

void _ShipDrawPathLine(const TimeLineCoordinateData* tcd, int* x_pos, int* y_pos, entity_id_t to_planet, timemath::Time to_time, int x_offset) {
    int new_x = GetPlanetCoord(tcd, to_planet) + x_offset;
    int new_y = GetTimeCoord(tcd, to_time);
    DrawLine(*x_pos, *y_pos, new_x, new_y, TRANSFER_UI_COLOR);
    *x_pos = new_x;
    *y_pos = new_y;
}

void _DrawShips(TimeLineCoordinateData* tcd, const Ships* ships) {
    for (auto it = ships->alloc.GetIter(); it; it++) {
        const Ship* ship = ships->alloc.Get(it);
        int x_offset = (it.iterator + 1) * 4;

        int start_tp_index = 0;
        int end_point_x = 0;
        int end_point_y = GetTimeCoord(tcd, GlobalGetNow());

        entity_id_t end_planet = GetInvalidId();
        if (ship->prepared_plans_count > 0 && !(ship->prepared_plans_count == 1 && ship->plan_edit_index == 0)) {
            int last_index = ship->prepared_plans_count - 1;
            if (ship->plan_edit_index == last_index) last_index--;
            end_planet = ship->prepared_plans[last_index].arrival_planet;
        } else {
            end_planet = ship->parent_planet;
        }

        if (ship->is_parked) {
            end_point_x = GetPlanetCoord(tcd, ship->parent_planet) + x_offset;
            if (ship->prepared_plans_count > 0) {
                // vertical line to the first tp
                _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, ship->parent_planet, ship->prepared_plans[0].departure_time, x_offset);
            }
        } else {
            ASSERT(ship->prepared_plans_count > 0)
            const TransferPlan* tp = &ship->prepared_plans[0];
            double travel_progress = 1 - 
                (tp->arrival_time - GlobalGetNow()).Seconds() / 
                (tp->arrival_time - tp->departure_time).Seconds();
            
            DEBUG_SHOW_F(travel_progress)

            end_point_x = Lerp(GetPlanetCoord(tcd, tp->departure_planet), GetPlanetCoord(tcd, tp->arrival_planet), travel_progress) + x_offset;
            _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp->arrival_planet, tp->arrival_time, x_offset);

            start_tp_index++;
        }
        for(int i=start_tp_index; i < ship->prepared_plans_count; i++) {
            const TransferPlan* tp = &ship->prepared_plans[i];

            if (i == ship->plan_edit_index && !IsIdValid(tp->arrival_planet)) {
                continue;
            }

            // transfer plan itself
            _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp->arrival_planet, tp->arrival_time, x_offset);
            // vertical line to the next tp
            if (i != ship->prepared_plans_count - 1) {
                const TransferPlan* tp_next = &ship->prepared_plans[i+1];
                _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp_next->departure_planet, tp_next->departure_time, x_offset);
            }
        }
        _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, end_planet, GetEndTime(tcd), x_offset);
    }
}

bool show_timeline = true;

bool TimelineShown() {
    return show_timeline;
}

void TimelineHide() {
    show_timeline = false;
}

void DrawTimeline() {
    // Manage viewing
    if (IsKeyPressed(KEY_W)) {
        show_timeline = !show_timeline;
    }
    if (!show_timeline) return;

    // Create UI Context
    GlobalState* gs = GlobalGetState();
    UIContextCreate(20, 100, GetScreenWidth() - 40, GetScreenHeight() - 100, 16, WHITE);
    UIContextEnclose(BG_COLOR, MAIN_UI_COLOR);

    // Initiualize Common Data Structure
    TimeLineCoordinateData tcd = TimeLineCoordinateData();
    tcd.x0 = UIContextCurrent().text_start_x;
    tcd.y0 = UIContextCurrent().text_start_y;
    tcd.w  = UIContextCurrent().width;
    tcd.h  = UIContextCurrent().height;
    tcd.planet_coords = new int[gs->planets.planet_count];

    _DrawPlanets(&tcd, &gs->planets);
    _DrawQuests(&tcd, &gs->quest_manager);
    _DrawShips(&tcd, &gs->ships);

    delete[] tcd.planet_coords;

    UIContextPop();  // Timieline
}