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
    // Macro and micro scale represent scales on which ships and planets are rendered

    static constexpr double macro_scale = 1e8;  // m per game unit
    static constexpr double micro_scale = 1e2;  // m per game unit
    static constexpr double scale_ratio = macro_scale / micro_scale;

    static Vector3 WorldToMacro(DVector3 world) { return (Vector3) (world / macro_scale); }
    static float WorldToMacro(double world) { return (float) (world / macro_scale); }
    static Vector3 WorldToMicro(DVector3 world) { return (Vector3) (world / micro_scale); }
    static float WorldToMicro(double world) { return (float) (world / micro_scale); }

    RID focus_object;

    DVector3 world_focus;
    Vector3 view_direction;
    double macro_view_distance;

    Camera3D macro_camera;
    Camera3D micro_camera;

    double MacroNearClipPlane() const;
    double MacroFarClipPlane() const;
    double MicroNearClipPlane() const;
    double MicroFarClipPlane() const;

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