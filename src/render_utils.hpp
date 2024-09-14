#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"
#include "string_builder.hpp"
#include "basic.hpp"
#include "assets.hpp"

struct OrbitSegment;
struct Text3D;

namespace orbit_render_mode {
    enum T {
        Solid = 0,
        Gradient = 1,
        Dashed = 2
    };
}

bool ShaderNeedReload(Shader shader);
void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer);

void BeginRenderSDFInUILayer(uint8_t z_layer, Color background);
void BeginRenderInUILayer(uint8_t z_layer, Color background, bool raw_color=false);
void BeginRenderInUILayer(uint8_t z_layer);
void EndRenderInUILayer();

void RenderWireframeMesh(WireframeMesh mesh, Matrix transform, Color background, Color foreground);
void RenderWireframeMesh2D(WireframeMesh mesh, Rectangle box, Color foreground, uint8_t z_layer);
void RenderWireframeMesh2DEx(WireframeMesh mesh, Vector2 mid_pos, float scale, Color background, Color foreground, uint8_t z_layer);
void RenderOrbit(const OrbitSegment* orbit, orbit_render_mode::T render_mode, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);
void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color);
void RenderSkyBox();
void RenderDeferred(RenderTexture render_target);

struct AtlasPos;

#define RELOAD_IF_NECESSARY(shader_name) if (!assets::IsShaderLoaded(shader_name::shader)) { shader_name::Load(); }
#define LOAD_SHADER(shader_name) shader_name::shader = assets::GetShader("resources/shaders/"#shader_name);
#define LOAD_SHADER_UNIFORM(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocation(shader_name::shader, #uniform_name);
#define LOAD_SHADER_ATTRIB(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocationAttrib(shader_name::shader, #uniform_name);

#endif  // RENDER_UTILS_H