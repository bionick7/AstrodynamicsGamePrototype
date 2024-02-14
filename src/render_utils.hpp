#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"
#include <inttypes.h>

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

int GetCharacterIndex(Vector2 position, Font font, const char *text, float fontSize, float spacing);
Rectangle GetTextRect(int token_start, int token_end, int token_from, Font font, const char *text, float fontSize, float spacing);

void InternalDrawText(const char *text, Vector2 position, Color color);
void InternalDrawTextEx(Font font, const char *text, Vector2 position, 
                        float fontSize, float spacing, Color tint);
void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, 
                        float spacing, Color tint, Rectangle render_rect, uint8_t z_layer);
void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer);

void BeginRenderInUIMode(uint8_t z_layer);
void EndRenderInUIMode();

void RenderOrbit(const OrbitSegment* orbit, int point_count, 
                 OrbitRenderMode::E render_mode, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);
void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color);
void RenderSkyBox();
void RenderDeferred(RenderTexture render_target);

struct AtlasPos;

#define RELOAD_IF_NECAISSARY(shader_name) if (!IsShaderReady(shader_name::shader)) { shader_name::Load(); }
#define LOAD_SHADER(shader_name) shader_name::shader = LoadShader("resources/shaders/"#shader_name".vs", "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_FS(shader_name) shader_name::shader = LoadShader(NULL, "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_UNIFORM(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocation(shader_name::shader, #uniform_name);

#endif  // RENDER_UTILS_H