#ifndef COORDINATE_TRANSFORM_H
#define COORDINATE_TRANSFORM_H

#include "basic.hpp"
#include "time.hpp"

struct Calendar {
    Time time;
    Time prev_time;

    Time current_migration_period;
    Time migration_arrrival_time;
    entity_id_t migration_arrrival_planet;  // Doesn't really fit anywhere right now ...

    float time_scale;
    bool paused;

    void Make(Time t0);
    Time AdvanceTime(double delta_t);
    void HandleInput(double delta_t);
    void DrawUI() const;
};

Calendar* GetCalendar();

struct CoordinateTransform {
    float space_scale;
    Vector2 focus;

    void Make();
    Vector2 TransformV(Vector2 p) const;
    Vector2 InvTransformV(Vector2 p) const;
    double TransformS(double p) const;
    double InvTransformS(double p) const;
    void TransformBuffer(Vector2* buffer, int buffer_size) const;
    void HandleInput(double delta_t);
};

Vector2 GetMousePositionInWorld();
CoordinateTransform* GetScreenTransform();

#endif  // COORDINATE_TRANSFORM