#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"

struct OrbitSegment;
struct Text3D;

namespace OrbitRenderMode {
    enum E {
        Solid = 0,
        Gradient = 1,
        Dashed = 2
    };
}

bool ShaderNeedReload(Shader shader);

void ReloadShaders();

void RenderOrbit(const OrbitSegment* orbit, int point_count, OrbitRenderMode::E render_mode, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);
void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color);
void RenderSkyBox();
void RenderDeferred(RenderTexture render_target);

struct AtlasPos;

#define RELOAD_IF_NECAISSARY(shader_name) if (!IsShaderReady(shader_name::shader)) { shader_name::Load(); }
#define LOAD_SHADER(shader_name) shader_name::shader = LoadShader("resources/shaders/"#shader_name".vs", "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_UNIFORM(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocation(shader_name::shader, #uniform_name);

#endif  // RENDER_UTILS_H