#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"

struct OrbitSegment;

namespace OrbitRenderMode {
    enum E {
        Solid = 0,
        Gradient = 1,
        Dashed = 2
    };
}

void ReloadShaders();

void RenderOrbit(const OrbitSegment* orbit, int point_count, OrbitRenderMode::E render_mode, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);
void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color);
void RenderSkyBox();

struct AtlasPos {
    int x, y;
    int size;

    AtlasPos(int x, int y, int size);
    AtlasPos() = default;
    Rectangle GetRect() const;
};


namespace rendering {
    Shader GetIconShader();
    Texture2D GetIconAtlas(int size);
}

#endif  // RENDER_UTILS_H