#include "time.hpp"
#include "timeline.hpp"
#include "global_state.hpp"
#include "constants.hpp"
#include "debug_drawing.hpp"
#include "string_builder.hpp"
#include "utils.hpp"
#include "diverse_ui.hpp"

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
    return tcd->y0 + 24 + (t - ref_time).Seconds() * pixels_per_day_vscale / timemath::SECONDS_IN_DAY;
}

timemath::Time GetEndTime(const TimeLineCoordinateData* tcd) {
    timemath::Time ref_time = GlobalGetNow();
    return ref_time + timemath::Time(((float)tcd->h - 24.f) / (float) pixels_per_day_vscale * timemath::SECONDS_IN_DAY);
}

void _DrawHohmanTFs(const TimeLineCoordinateData* tcd, RID from, RID to) {
    Orbit from_orbit = GetPlanet(from)->orbit;
    Orbit to_orbit = GetPlanet(to)->orbit;
    timemath::Time t0 = GlobalGetNow();
    //timemath::Time t1 = t0 + timemath::Time::Day() * (tcd->h / pixels_per_day_vscale);
    //while (t0 < t1) {
    if (true) {
        timemath::Time departure_t;
        timemath::Time arrival_t;
        HohmannTransfer(&from_orbit, &to_orbit, t0, &departure_t, &arrival_t, NULL, NULL);
        DrawLine(
            GetPlanetCoord(tcd, from), GetTimeCoord(tcd, departure_t),
            GetPlanetCoord(tcd, to), GetTimeCoord(tcd, arrival_t),
            Palette::ally_alt
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
        //DebugPrintText("%s: sma = %f, x = %d", planet->name.GetChar(), planet->orbit.sma, x);
        Rectangle rect = DrawTextAligned(
            planet->name.GetChar(), {(float)x, (float)tcd->y0 + 25}, 
            text_alignment::HCENTER | text_alignment::BOTTOM, 
            Palette::ui_main, Palette::bg, ui::Current()->z_layer
        );
        //DebugPrintText("%f, %f; %f, %f", rect.x, rect.y, rect.width, rect.height);
        //DrawRectangleRec(rect, RED);
        if(CheckCollisionPointRec(GetMousePosition(), rect)) {
            mouse_hover_planet = planet_index;
        }
        ui::BeginDirectDraw();
        DrawLine(x, tcd->y0 + 24, x, tcd->y0 + tcd->h, Palette::ui_main);
        ui::EndDirectDraw();
        previous_x = x;
        tcd->planet_coords[planet_index] = x;
    }

    // Draw 'y-backticks'
    int t_indx = 0;
    StringBuilder sb;
    timemath::Time day_start = timemath::Time(((int)GlobalGetNow().Days() + 1) * timemath::SECONDS_IN_DAY);
    int time_interval = 1;
    if (pixels_per_day_vscale < 40) time_interval = 7;
    if (pixels_per_day_vscale < 4) time_interval = 31;
    for (timemath::Time t = day_start; t < GetEndTime(tcd); t = t + timemath::Time::Day()) {
        int y = GetTimeCoord(tcd, t);
        if (t_indx++ % time_interval == 0) {
            sb.Clear();
            DrawTextAligned(
                sb.AddDate(t, true).c_str, 
                { (float) tcd->x0 + 20, (float) y },
                text_alignment::BOTTOM | text_alignment::LEFT,
                Palette::ui_alt, Palette::bg,
                ui::Current()->z_layer
            );
        }
    }
    
    ui::BeginDirectDraw();
    for (timemath::Time t = day_start; t < GetEndTime(tcd); t = t + timemath::Time::Day()) {
        int y = GetTimeCoord(tcd, t);
        DrawLine(tcd->x0 - 2, y, tcd->x0 + 15, y, Palette::ui_main);
        DrawLine(tcd->x0 + 40, y, tcd->x0 + tcd->w - 10, y, Palette::ui_dark);
    }
    ui::EndDirectDraw();

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

void _ShipDrawPathLine(const TimeLineCoordinateData* tcd, int* x_pos, int* y_pos, 
                       RID to_planet, timemath::Time to_time, int x_offset, Color color) {
    int new_x = GetPlanetCoord(tcd, to_planet) + x_offset;
    int new_y = GetTimeCoord(tcd, to_time);
    DrawLine(*x_pos, *y_pos, new_x, new_y, color);
    *x_pos = new_x;
    *y_pos = new_y;
}

void _DrawShips(TimeLineCoordinateData* tcd, const Ships* ships) {
    ui::BeginDirectDraw();
    for (auto it = ships->alloc.GetIter(); it; it++) {
        const Ship* ship = ships->alloc.Get(it);
        int index_on_planet = 0;
        int x_offset = (index_on_planet + 1) * 4;

        int start_tp_index = 0;
        int end_point_x = 0;
        int end_point_y = GetTimeCoord(tcd, GlobalGetNow());

        if (!ship->IsLeading()) {
            // TODO: indicate rest of fleet
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
            // vertical line to the next tp
            if (ship->prepared_plans_count > 1) {
                const TransferPlan* tp_next = &ship->prepared_plans[1];
                _ShipDrawPathLine(tcd, &end_point_x, &end_point_y, tp_next->departure_planet, tp_next->departure_time, x_offset, ship->GetColor());
            }

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
    ui::EndDirectDraw();
}

void DrawTimeline() {
    // Create UI Context
    GlobalState* gs = GetGlobalState();
    ui::CreateNew(
        20, 100, GetScreenWidth() - 40, GetScreenHeight() - 100, 
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::MENU_PANELS
    );
    ui::Enclose();

    // Initiualize Common Data Structure
    TimeLineCoordinateData tcd = TimeLineCoordinateData();
    tcd.x0 = ui::Current()->x;
    tcd.y0 = ui::Current()->y;
    tcd.w  = ui::Current()->width;
    tcd.h  = ui::Current()->height;
    tcd.planet_coords = new int[gs->planets.planet_count];

    // Scrolling
    if (panel_management::GetCurrentFocus() == Focusables::TIMELINE) {
        float scroll_ratio = 1 + 0.1 * GetMouseWheelMove();
        if (scroll_ratio > 1) {
            pixels_per_day_vscale = ClampInt(pixels_per_day_vscale * scroll_ratio, pixels_per_day_vscale + 1, tcd.h);
        } else  if (scroll_ratio < 1) {
            pixels_per_day_vscale = ClampInt(pixels_per_day_vscale * scroll_ratio, 3, pixels_per_day_vscale - 1);
        }
    }

    _DrawPlanets(&tcd, &gs->planets);
    _DrawShips(&tcd, &gs->ships);

    delete[] tcd.planet_coords;

    ui::Pop();  // Timieline
}
