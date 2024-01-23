#ifndef COORDINATE_TRANSFORM_H
#define COORDINATE_TRANSFORM_H

#include "basic.hpp"
#include "time.hpp"

struct Calendar {
    timemath::Time time;
    timemath::Time prev_time;

    //RID migration_arrrival_planet;  // Doesn't really fit anywhere right now ...

    double time_scale;
    bool paused;

    void Make(timemath::Time t0);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    timemath::Time AdvanceTime(double delta_t);
    void HandleInput(double delta_t);
    void DrawUI() const;

    bool IsNewDay() const;
    timemath::Time GetFrameElapsedGameTime() const;
};

Calendar* GetCalendar();

struct CoordinateTransform {
    float space_scale;
    Vector2 focus;

    void Make();
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

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