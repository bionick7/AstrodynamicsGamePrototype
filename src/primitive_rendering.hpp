#ifndef PRIMITIVE_RENDERING_H
#define PRIMITIVE_RENDERING_H

#include "render_utils.hpp"
#include "time.hpp"

namespace orbit_render_mode {
    enum T {
        Solid = 0,
        Gradient = 1,
        Dashed = 2
    };
}

struct OrbitSegment;
struct Orbit;

struct ConicRenderInfo {
    Vector3 center;
    float semi_latus_rectum;
    float eccentricity;
    Matrix orientation;

    float focal_bound;
    float focal_range;
    float anomaly;

    orbit_render_mode::T render_mode;
    Color color;

    static ConicRenderInfo FromOrbitSegment(const OrbitSegment* segment, 
                                           orbit_render_mode::T render_mode, Color color);
    static ConicRenderInfo FromOrbit(const Orbit *orbit, timemath::Time time,
                                     orbit_render_mode::T render_mode, Color color);
    static ConicRenderInfo FromCircle(DVector3 world_position, Matrix orientation, double radius, 
                                      orbit_render_mode::T render_mode, Color color);
};

struct SphereRenderInfo {
    Vector3 center;
    float radius;
    Color color;
    
    static SphereRenderInfo FromWorldPos(DVector3 center, double radius, Color color);
};

void RenderOrbits(const ConicRenderInfo orbit[], int number);
void RenderPerfectSpheres(const SphereRenderInfo orbit[], int number);

#endif  // PRIMITIVE_RENDERING_H