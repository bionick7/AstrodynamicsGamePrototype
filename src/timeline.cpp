#include "time.hpp"
#include "timeline.hpp"
#include "global_state.hpp"
#include "constants.hpp"
#include "debug_drawing.hpp"
#include "string_builder.hpp"
#include "utils.hpp"

bool show_timeline = false;
int pixels_per_day_vscale = 60;

struct TimeLineCoordinateData {
    int x0;
    int y0;
    int w;
    int h;

    int* planet_coords;
};

int GetPlanetCoord(const TimeLineCoordinateData* tcd, RID planet) {
    return tcd->planet_coords[IdGetIndex(planet)];
}

int GetTimeCoord(const TimeLineCoordinateData* tcd, timemath::Time t) {
    timemath::Time ref_time = GlobalGetNow();
    return tcd->y0 + 24 + (t - ref_time).Seconds() * pixels_per_day_vscale / 86400;
}

timemath::Time GetEndTime(const TimeLineCoordinateData* tcd) {
    timemath::Time ref_time = GlobalGetNow();
    return ref_time + timemath::Time(((float)tcd->h - 24.f) / (float) pixels_per_day_vscale * 86400);
}

void _DrawHohmanTFs(const TimeLineCoordinateData* tcd, RID from, RID to) {
    Orbit from_orbit = GetPlanet(from)->orbit;
    Orbit to_orbit = GetPlanet(to)->orbit;
    timemath::Time t0 = GlobalGetNow();
    timemath::Time t1 = t0 + timemath::Time::Day() * (tcd->h / pixels_per_day_vscale);
    //while (t0 < t1) {
    if (true) {
        timemath::Time departure_t;
        timemath::Time arrival_t;
        HohmannTransfer(&from_orbit, &to_orbit, t0, &departure_t, &arrival_t, NULL, NULL);
        DrawLine(
            GetPlanetCoord(tcd, from), GetTimeCoord(tcd, departure_t),
            GetPlanetCoord(tcd, to), GetTimeCoord(tcd, arrival_t),
            Palette::ship_alt
        );
        t0 = departure_t + 1;
    }
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

    int previous_x = 40;
    int mouse_hover_planet = -1;
    for (int planet_index = 0; planet_index < planets->planet_count; planet_index++) {
        const Planet* planet = &planets->planet_array[planet_index];
        double ratio = log(planet->orbit.sma/min_sma) / log(max_sma/min_sma);
        int x = ratio * (tcd->w - min_planet_spacing) + tcd->x0;
        if (x - previous_x < min_planet_spacing) {
            x = previous_x + min_planet_spacing;
        }
        //DebugPrintText("%s: sma = %f, x = %d", planet->name, planet->orbit.sma, x);
        Rectangle rect = DrawTextAligned(planet->name, {(float)x, (float)tcd->y0 + 18}, TextAlignment::HCENTER | TextAlignment::BOTTOM, Palette::ui_main);
        if(CheckCollisionPointRec(GetMousePosition(), rect)) {
            mouse_hover_planet = planet_index;
        }
        DrawLine(x, tcd->y0 + 24, x, tcd->y0 + tcd->h, Palette::ui_main);
        previous_x = x;
        tcd->planet_coords[planet_index] = x;
    }

    // Draw 'y-backticks'
    int t_indx = 0;
    StringBuilder sb;
    timemath::Time day_start = timemath::Time(((int)GlobalGetNow().Days() + 1) * 86400);
    int time_interval = 1;
    if (pixels_per_day_vscale < 40) time_interval = 7;
    if (pixels_per_day_vscale < 4) time_interval = 31;
    for (timemath::Time t = day_start; t < GetEndTime(tcd); t = t + timemath::Time::Day()) {
        int y = GetTimeCoord(tcd, t);
        DrawLine(tcd->x0 - 2, y, tcd->x0 + 15, y, Palette::ui_main);
        if (t_indx++ % time_interval == 0) {
            DrawTextAligned(
                sb.Clear().AddDate(t, true).c_str, 
                { (float) tcd->x0 + 20, (float) y },
                TextAlignment::BOTTOM | TextAlignment::LEFT,
                Palette::ui_alt
            );
        }
        DrawLine(tcd->x0 + 40, y, tcd->x0 + tcd->w - 10, y, Palette::ui_dark);
    }

    if (mouse_hover_planet >= 0) {
        for (int planet_index = 0; planet_index < planets->planet_count; planet_index++) {
            if (planet_index != mouse_hover_planet)
                _DrawHohmanTFs(tcd, RID(mouse_hover_planet, EntityType::PLANET), RID(planet_index, EntityType::PLANET));
        }
    }
}

float _SDSegment(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 pa = Vector2Subtract(p, a);
    Vector2 ba = Vector2Subtract(b, a);
    float h = Clamp(Vector2DotProduct(pa, ba) / Vector2DotProduct(ba, ba), 0.0, 1.0 );
    return Vector2Distance(pa, Vector2Scale(ba, h));
}

float  _QuestDrawLine(TimeLineCoordinateData* tcd, const Task* q, bool active) {
    // returns the mouse distance to the line (in pixels)
    RID from = q->departure_planet;
    RID to = q->arrival_planet;
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

    Color c = Palette::ui_main;
    if (!active) {
        c = ColorAlphaBlend(Palette::bg, c, GetColor(0x80808080u));
    }

    //Vector2 scaled_unit = Vector2Scale(Vector2Normalize(Vector2Subtract(end_pos, start_pos)), 10);
    Vector2 scaled_unit = {(end_pos.x - start_pos.x) > 0 ? 10 : -10, 0};
    DrawTextAligned(
        StringBuilder().AddI(KGToResourceCounts(q->payload_mass)).c_str, 
        Vector2Lerp(start_pos, end_pos, 0.15), 
        TextAlignment::HCENTER | TextAlignment::BOTTOM, c
    );
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

    return _SDSegment(GetMousePosition(), start_pos, end_pos);

    /*Vector2 start_control = start_pos;
    start_control.x = (start_pos.x * 2 + end_pos.x) / 3;
    Vector2 end_control = end_pos;
    end_control.x = (end_pos.x * 2 + start_pos.x) / 3;
    DrawLineBezierCubic(start_pos, end_pos,
        start_control, end_control,
        1, c
    );*/
}

void _DrawQuests(TimeLineCoordinateData* tcd, QuestManager* qm) {
    float closest_mouse_dist = INFINITY;
    RID closest_mouse_dist_quest = GetInvalidId();
    bool closest_mouse_dist_quest_is_active = false;
    /*for(int i=0; i < qm->GetAvailableQuests(); i++) {
        const Quest* q = qm->available_quests[i];
        if (!q->IsValid()) {
            continue;
        }
        float mouse_dist = _QuestDrawLine(tcd, q, false);
        if (mouse_dist < closest_mouse_dist) {
            closest_mouse_dist = mouse_dist;
            closest_mouse_dist_quest = i;
            closest_mouse_dist_quest_is_active = false;
        }
    }*/
    for(auto it = qm->active_tasks.GetIter(); it; it++) {
        float mouse_dist = _QuestDrawLine(tcd, qm->active_tasks.Get(it), true);
        if (mouse_dist < closest_mouse_dist) {
            closest_mouse_dist = mouse_dist;
            closest_mouse_dist_quest = it.GetId();
            closest_mouse_dist_quest_is_active = true;
        }
    }
    if (closest_mouse_dist < 15) {
        UIContextPushAligned(400, 100, TextAlignment::TOP | TextAlignment::RIGHT);
        if (closest_mouse_dist_quest_is_active) {
            qm->active_tasks.Get(closest_mouse_dist_quest)->DrawUI(false, false);
        } else {
            qm->available_quests.Get(closest_mouse_dist_quest)->DrawUI(false, false);
            UIContextWrite("Press enter to accept quest", true);
            if (IsKeyPressed(KEY_ENTER)) {
                HandleButtonSound(ButtonStateFlags::JUST_PRESSED);
                qm->AcceptQuest(closest_mouse_dist_quest);
            }
        }
        UIContextPop();
    }
}

void _ShipDrawPathLine(const TimeLineCoordinateData* tcd, int* x_pos, int* y_pos, 
                       RID to_planet, timemath::Time to_time, int x_offset, Color color) {
    int new_x = GetPlanetCoord(tcd, to_planet) + x_offset;
    int new_y = GetTimeCoord(tcd, to_time);
    DrawLine(*x_pos, *y_pos, new_x, new_y, color);
    *x_pos = new_x;
    *y_pos = new_y;
}

void _DrawShips(TimeLineCoordinateData* tcd, const Ships* ships) {
    for (auto it = ships->alloc.GetIter(); it; it++) {
        const Ship* ship = ships->alloc.Get(it);
        int x_offset = (ship->index_on_planet + 1) * 4;

        int start_tp_index = 0;
        int end_point_x = 0;
        int end_point_y = GetTimeCoord(tcd, GlobalGetNow());

        if (!ship->IsLeading()) {
            // TODO
            continue;
        }

        if (ship->IsParked()) {
            end_point_x = GetPlanetCoord(tcd, ship->GetParentPlanet()) + x_offset;
            if (ship->prepared_plans_count > 0) {
                // vertical line to the first tp
                _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, ship->GetParentPlanet(), ship->prepared_plans[0].departure_time, x_offset, ship->GetColor());
            }
        } else {
            ASSERT(ship->prepared_plans_count > 0)
            const TransferPlan* tp = &ship->prepared_plans[0];
            double travel_progress = 1 - 
                (tp->arrival_time - GlobalGetNow()).Seconds() / 
                (tp->arrival_time - tp->departure_time).Seconds();
            
            end_point_x = Lerp(GetPlanetCoord(tcd, tp->departure_planet), GetPlanetCoord(tcd, tp->arrival_planet), travel_progress) + x_offset;
            _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp->arrival_planet, tp->arrival_time, x_offset, ship->GetColor());

            start_tp_index++;
        }

        for(int i=start_tp_index; i < ship->prepared_plans_count; i++) {
            const TransferPlan* tp = &ship->prepared_plans[i];

            if (!ship->IsTrajectoryKnown(i)) continue;
            if (i == ship->plan_edit_index && !IsIdValid(tp->arrival_planet)) continue;

            // transfer plan itself
            _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp->arrival_planet, tp->arrival_time, x_offset, ship->GetColor());
            // vertical line to the next tp
            if (i != ship->prepared_plans_count - 1) {
                const TransferPlan* tp_next = &ship->prepared_plans[i+1];
                _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp_next->departure_planet, tp_next->departure_time, x_offset, ship->GetColor());
            }
        }

        DrawLine(end_point_x, end_point_y, end_point_x, GetTimeCoord(tcd, GetEndTime(tcd)), ship->GetColor());
        //_ShipDrawPathLine(tcd, &end_point_x, &end_point_y, end_planet, GetEndTime(tcd), x_offset, ship->GetColor());
    }
}

bool TimelineIsOpen() {
    return show_timeline;
}

void TimelineClose() {
    show_timeline = false;
}

void DrawTimeline() {

    // Manage viewing
    if (IsKeyPressed(KEY_W)) {
        show_timeline = !show_timeline;
    }
    if (!show_timeline) return;

    // Create UI Context
    GlobalState* gs = GetGlobalState();
    UIContextCreateNew(20, 100, GetScreenWidth() - 40, GetScreenHeight() - 100, 16, Palette::ui_main);
    UIContextEnclose(Palette::bg, Palette::ui_main);

    // Initiualize Common Data Structure
    TimeLineCoordinateData tcd = TimeLineCoordinateData();
    tcd.x0 = UIContextCurrent().text_start_x;
    tcd.y0 = UIContextCurrent().text_start_y;
    tcd.w  = UIContextCurrent().width;
    tcd.h  = UIContextCurrent().height;
    tcd.planet_coords = new int[gs->planets.planet_count];

    // Scrolling
    GetUI()->scroll_lock = true;
    if (GetGlobalState()->current_focus == GlobalState::TIMELINE) {
        float scroll_ratio = 1 + 0.1 * GetMouseWheelMove();
        if (scroll_ratio > 1) {
            pixels_per_day_vscale = ClampInt(pixels_per_day_vscale * scroll_ratio, pixels_per_day_vscale + 1, tcd.h);
        } else  if (scroll_ratio < 1) {
            pixels_per_day_vscale = ClampInt(pixels_per_day_vscale * scroll_ratio, 3, pixels_per_day_vscale - 1);
        }
    }

    _DrawPlanets(&tcd, &gs->planets);
    _DrawQuests(&tcd, &gs->quest_manager);
    _DrawShips(&tcd, &gs->ships);

    delete[] tcd.planet_coords;

    UIContextPop();  // Timieline
}
