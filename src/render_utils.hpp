#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"
#include "string_builder.hpp"
#include "basic.hpp"

struct OrbitSegment;
struct Text3D;

namespace OrbitRenderMode {
    enum T {
        Solid = 0,
        Gradient = 1,
        Dashed = 2
    };
}

bool ShaderNeedReload(Shader shader);
void ReloadShaders();
void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer);

void BeginRenderInUIMode(uint8_t z_layer);
void EndRenderInUIMode();

void RenderOrbit(const OrbitSegment* orbit, int point_count, 
                 OrbitRenderMode::T render_mode, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);
void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color);
void RenderSkyBox();
void RenderDeferred(RenderTexture render_target);

struct AtlasPos;

#define RELOAD_IF_NECAISSARY(shader_name) if (!IsShaderReady(shader_name::shader)) { shader_name::Load(); }
#define LOAD_SHADER(shader_name) shader_name::shader = LoadShader("resources/shaders/"#shader_name".vs", "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_FS(shader_name) shader_name::shader = LoadShader(NULL, "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_UNIFORM(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocation(shader_name::shader, #uniform_name);

namespace sdf_shader {
    extern Shader shader;
    extern int depth;
    void Load();
    void UnLoad();
}

#endif  // RENDER_UTILS_H