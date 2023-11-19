#ifndef COORDINATE_TRANSFORM_H
#define COORDINATE_TRANSFORM_H

#include "basic.hpp"
#include "time.hpp"

struct CoordinateTransform {
    float space_scale;
    float time_scale;
    bool paused;
    Vector2 focus;

    void Make();
    Vector2 TransformV(Vector2 p) const;
    Vector2 InvTransformV(Vector2 p) const;
    double TransformS(double p) const;
    double InvTransformS(double p) const;
    void TransformBuffer(Vector2* buffer, int buffer_size) const;
    Time AdvanceTime(Time t0, double delta_t) const;
    void HandleInput(double delta_t);
    void DrawUI() const;
};

Vector2 GetMousePositionInWorld();

CoordinateTransform* GetScreenTransform();

#endif  // COORDINATE_TRANSFORM