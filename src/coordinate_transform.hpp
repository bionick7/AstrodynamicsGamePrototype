#ifndef COORDINATE_TRANSFORM_H
#define COORDINATE_TRANSFORM_H

#include "dvector3.hpp"
#include "basic.hpp"
#include "time.hpp"
#include "id_system.hpp"

struct Calendar {
    timemath::Time time;
    timemath::Time prev_time;

    double time_scale;
    bool paused;

    bool timelapse = false;
    timemath::Time timelapse_origin;
    timemath::Time timelapse_target;

    void Make(timemath::Time t0);
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void FastForward(timemath::Time target);

    void Update(double delta_t);
    void DrawUI() const;

    bool IsNewDay() const;
    timemath::Time GetFrameElapsedGameTime() const;
};

struct GameCamera {
    // m per game unit
    static constexpr double space_scale = 1e8;
    static Vector3 WorldToRender(DVector3 world) { return (Vector3) (world / space_scale); }
    static float WorldToRender(double world) { return (float) (world / space_scale); }

    RID focus_object;
    Camera3D rl_camera;

    void Make();
    
    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void HandleInput();
    bool IsInView(Vector3 render_pos) const;
    bool IsInView(DVector3 render_pos) const;
    Vector2 GetScreenPos(DVector3 world_pos) const;
    float MeasurePixelSize(Vector3 render_pos) const;

    Matrix ViewMatrix() const;
    Matrix ProjectionMatrix() const;
};

#endif  // COORDINATE_TRANSFORM