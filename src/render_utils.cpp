#include "render_utils.hpp"
#include "primitive_rendering.hpp"

#include "global_state.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "ui.hpp"
#include "debug_console.hpp"

#include "rlgl.h"

void _RenderQuad(Color color) {
    rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(-1.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f( 1.0f,  1.0f, 0.0f);
    rlEnd();
}

void _RenderDoubleQuad(Color color) {
    rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(-1.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f( 1.0f,  1.0f, 0.0f);

        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(-1.0f, -1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(-1.0f,  1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f( 1.0f,  1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f( 1.0f, -1.0f, 0.0f);
    rlEnd();
}

void ColorToFloatBuffer(float buffer[], Color color) {
    // Assumes the buffer has at least 4 floats allocated to it
    buffer[0] = (float)color.r / 255.0f;
    buffer[1] = (float)color.g / 255.0f;
    buffer[2] = (float)color.b / 255.0f;
    buffer[3] = (float)color.a / 255.0f;
}

void Vector3ToFloatBuffer(float buffer[], Vector3 vec) {
    buffer[0] = vec.x;
    buffer[1] = vec.y;
    buffer[2] = vec.z;
}
namespace sdf_shader {
    Shader shader;
    int depth = -1;
    int background_color = -1;

    void Load() {
        LOAD_SHADER(sdf_shader)
        LOAD_SHADER_UNIFORM(sdf_shader, depth)
        LOAD_SHADER_UNIFORM(sdf_shader, background_color)
    }
}

namespace ui_shader {
    Shader shader;
    int depth = -1;
    int background_color = -1;
    int raw_color = -1;

    void Load() {
        LOAD_SHADER(ui_shader)
        LOAD_SHADER_UNIFORM(ui_shader, depth)
        LOAD_SHADER_UNIFORM(ui_shader, background_color)
        LOAD_SHADER_UNIFORM(ui_shader, raw_color)
    }
}

void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer) {
    RELOAD_IF_NECESSARY(sdf_shader)
    BeginShaderMode(sdf_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float bg_color_4[4];
    ColorToFloatBuffer(bg_color_4, Palette::bg);
    SetShaderValue(sdf_shader::shader, sdf_shader::background_color, bg_color_4, SHADER_UNIFORM_VEC4);
    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
    DrawTexturePro(texture, source, dest, origin, rotation, tint);
    EndShaderMode();
}

void BeginRenderSDFInUILayer(uint8_t z_layer, Color background) {
    RELOAD_IF_NECESSARY(sdf_shader)
    BeginShaderMode(sdf_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float bg_color_4[4];
    ColorToFloatBuffer(bg_color_4, background);
    SetShaderValue(sdf_shader::shader, sdf_shader::background_color, bg_color_4, SHADER_UNIFORM_VEC4);
    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
}

void BeginRenderInUILayer(uint8_t z_layer, Color background, bool raw_color) {
    RELOAD_IF_NECESSARY(ui_shader)
    BeginShaderMode(ui_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float bg_color_4[4];
    ColorToFloatBuffer(bg_color_4, background);  // Hardcoded for now
    int raw_color_int = (int) raw_color;
    SetShaderValue(ui_shader::shader, ui_shader::raw_color, &raw_color_int, SHADER_UNIFORM_INT);
    SetShaderValue(ui_shader::shader, ui_shader::background_color, bg_color_4, SHADER_UNIFORM_VEC4);
    SetShaderValue(ui_shader::shader, ui_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
}

void BeginRenderInUILayer(uint8_t z_layer) {
    BeginRenderInUILayer(z_layer, BLANK);
}

void EndRenderInUILayer() {
    EndShaderMode();
}

namespace wireframe_shader {
    Shader shader;

    int color = -1;
    int render_mode = -1;
    int mvp = -1;
    int time = -1;
    int depth = -1;
    int depth_offset = -1;

    void Load() {
        LOAD_SHADER(wireframe_shader)
        LOAD_SHADER_UNIFORM(wireframe_shader, render_mode)
        LOAD_SHADER_UNIFORM(wireframe_shader, color)
        LOAD_SHADER_UNIFORM(wireframe_shader, mvp)
        LOAD_SHADER_UNIFORM(wireframe_shader, time)
        LOAD_SHADER_UNIFORM(wireframe_shader, depth)
        LOAD_SHADER_UNIFORM(wireframe_shader, depth_offset)
    }
}

void RenderWireframeMesh(WireframeMesh mesh, Matrix transform, Color background, Color foreground) {
    RELOAD_IF_NECESSARY(wireframe_shader)

    static float bg_color[4];
    static float fg_color[4];
    ColorToFloatBuffer(bg_color, background);
    ColorToFloatBuffer(fg_color, foreground);

    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();
    Matrix matModel = MatrixMultiply(transform, rlGetMatrixTransform());
    Matrix matModelView = MatrixMultiply(matModel, matView);

    if (fabs(matModelView.m14) > 1000) 
        return;  // Distance culling

    Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);

    static int render_mode_lines = 0;
    static int render_mode_faces = 3;
    static float default_depth = -1.0;
    static float outline_offset = 1.0;
    static float zero_f = 0.0;

    rlEnableShader(wireframe_shader::shader.id);
    rlSetLineWidth(2.0f);
    //rlEnableSmoothLines();

    // Draw outline triangles
    rlSetUniformMatrix(wireframe_shader::mvp, matModelViewProjection);
    rlSetUniform(wireframe_shader::depth, &default_depth, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::depth_offset, &outline_offset, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_faces, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, fg_color, RL_SHADER_UNIFORM_VEC4, 1);

    rlEnableVertexArray(mesh.vao_triangles);
    rlEnableWireMode();

    rlDrawVertexArrayElements(0, mesh.triangle_count * 3, 0);

    rlDisableWireMode();
    rlDisableVertexArray();

    // Draw blockout triangles
    rlSetUniformMatrix(wireframe_shader::mvp, matModelViewProjection);
    rlSetUniform(wireframe_shader::depth, &default_depth, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::depth_offset, &zero_f, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_faces, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, bg_color, RL_SHADER_UNIFORM_VEC4, 1);

    rlEnableVertexArray(mesh.vao_triangles);
    rlDrawVertexArrayElements(0, mesh.triangle_count * 3, 0);
    rlDisableVertexArray();

    // Draw lines
    rlSetUniform(wireframe_shader::render_mode, &render_mode_lines, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, fg_color, RL_SHADER_UNIFORM_VEC4, 1);
    rlSetUniform(wireframe_shader::depth, &default_depth, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::depth_offset, &zero_f, RL_SHADER_UNIFORM_FLOAT, 1);

    rlSetLineWidth(1.0f);
    rlEnableVertexArray(mesh.vao_lines);
    rlDrawVertexPrimitiveArrayElements(RL_LINES, 0, mesh.line_count * 2, 0);
    rlDisableVertexArray();
    
    rlDisableShader();
}

void RenderWireframeMesh2DEx(WireframeMesh mesh, Vector2 origin, float scale, 
                             Color background, Color foreground, uint8_t z_layer) {
    RELOAD_IF_NECESSARY(wireframe_shader)

    static float bg_color[4];
    static float fg_color[4];
    ColorToFloatBuffer(bg_color, background);
    ColorToFloatBuffer(fg_color, foreground);

    // model to pixel-space 
    //Matrix model2ndc_matrix = MatrixScale(2.0, 1, 1);
    Matrix model2ndc_matrix = MatrixMultiply(MatrixScale(scale, scale, scale), MatrixTranslate(origin.x, 0, origin.y));
    // pixel-space to ndc
    model2ndc_matrix = MatrixMultiply(model2ndc_matrix, {
        2.0f / GetScreenWidth(), 0, 0, -1,
        0, 0, -2.0f / GetScreenHeight(), 1,
        0, 0, 0, 0,
        0, 0, 0, 1
    });

    int render_mode_lines = 0;
    int render_mode_faces = 3;
    int render_mode = 0;
    float zero_f = 0.0f;
    float time = GetTime();
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float z_layer_f_lines = 1.0f - (z_layer + 1) / 256.0f;

    rlEnableShader(wireframe_shader::shader.id);

    rlSetUniform(wireframe_shader::time, &time, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniformMatrix(wireframe_shader::mvp, model2ndc_matrix);

    rlSetUniform(wireframe_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::depth_offset, &zero_f, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_faces, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, bg_color, RL_SHADER_UNIFORM_VEC4, 1);
    rlDisableBackfaceCulling();

    rlEnableVertexArray(mesh.vao_triangles);
    rlDrawVertexArrayElements(0, mesh.triangle_count*3, 0);
    rlDisableVertexArray();

    rlSetUniform(wireframe_shader::depth_offset, &z_layer_f_lines, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_lines, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, fg_color, RL_SHADER_UNIFORM_VEC4, 1);

    rlEnableVertexArray(mesh.vao_lines);
    rlDrawVertexPrimitiveArrayElements(RL_LINES, 0, mesh.line_count*2, 0);
    rlDisableVertexArray();
    
    rlDisableShader();
}

void RenderWireframeMesh2D(WireframeMesh mesh, Rectangle box, Color color, uint8_t z_layer) {
    float scale = fmin(
        box.width / (mesh.bounding_box.max.x - mesh.bounding_box.min.x),
        box.height / (mesh.bounding_box.max.z - mesh.bounding_box.min.z)
    );
    float offset_x = box.x - mesh.bounding_box.min.x * scale;
    float offset_y = box.y - mesh.bounding_box.min.z * scale;
    RenderWireframeMesh2DEx(mesh, { offset_x, offset_y }, scale, color, Palette::bg, z_layer);
}

namespace rings_shader {
    Shader shader;

    int innerRad = -1;
    int bgColor = -1;
    
    void Load() {
        LOAD_SHADER(rings_shader)
        LOAD_SHADER_UNIFORM(rings_shader, innerRad)
        LOAD_SHADER_UNIFORM(rings_shader, bgColor)
    }
}

void RenderRings(DVector3 normal, double min_rad, double max_rad, Color color) {
    RELOAD_IF_NECESSARY(rings_shader)
    
    float inner_rad_float = min_rad / max_rad;
	SetShaderValue(rings_shader::shader, rings_shader::innerRad, &inner_rad_float, SHADER_ATTRIB_FLOAT);
    
    float bg_color4[4];
    ColorToFloatBuffer(bg_color4, Palette::bg);
	SetShaderValue(rings_shader::shader, rings_shader::bgColor, &bg_color4, SHADER_ATTRIB_VEC4);

    rlPushMatrix();
    // scale -> rotate -> translate
    float render_scale = GameCamera::WorldToMacro(max_rad);
    rlScalef(render_scale, render_scale, render_scale);
    rlRotatef(90, 1, 0, 0);
    rlTranslatef(0, 0, 0.001);

    BeginShaderMode(rings_shader::shader);
    _RenderDoubleQuad(color);
    EndShaderMode();

    rlPopMatrix();

    // Outlines
    float rads[2] = { min_rad, max_rad };
    for (int i=0; i < 2; i++)
        GetRenderServer()->QueueConicDraw(
            ConicRenderInfo::FromCircle(
                DVector3::Zero(), MatrixIdentity(), rads[i], orbit_render_mode::Solid, color)
        );
}

namespace skybox_shader {
    Shader shader;
    int resolution;
    int fov;
    int palette;
    int matCamera;
    int starMap;

    float palette_vec[4*4];
    
    void Load() {
        LOAD_SHADER(skybox_shader)
        LOAD_SHADER_UNIFORM(skybox_shader, resolution)
        LOAD_SHADER_UNIFORM(skybox_shader, fov)
        LOAD_SHADER_UNIFORM(skybox_shader, palette)
        LOAD_SHADER_UNIFORM(skybox_shader, matCamera)
        LOAD_SHADER_UNIFORM(skybox_shader, starMap)

        
        ColorToFloatBuffer(&palette_vec[4*0], Palette::bg);
        ColorToFloatBuffer(&palette_vec[4*1], Palette::ui_dark);
        ColorToFloatBuffer(&palette_vec[4*3], Palette::ui_alt);
        ColorToFloatBuffer(&palette_vec[4*2], Palette::ui_alt);
    }
}

void RenderSkyBox() {
    RELOAD_IF_NECESSARY(skybox_shader)

    Vector2 resolution = { GetScreenWidth(), GetScreenHeight() };
    
    float fov = GetCamera()->macro_camera.fovy * DEG2RAD;
    //Matrix camera_matrix = MatrixInvert(rlGetMatrixModelview());

    Vector3 mat_z = Vector3Subtract(GetCamera()->macro_camera.position, GetCamera()->macro_camera.target);  // In world space
	Vector3 mat_y = { 0.0f, 1.0f, 0.0f };  // Chosen arbitrarily
    Vector3OrthoNormalize(&mat_z, &mat_y);
    Vector3 mat_x = Vector3CrossProduct(mat_y, mat_z);
    Matrix camera_matrix = MatrixFromColumns(mat_x, mat_y, mat_z);

	SetShaderValue(skybox_shader::shader, skybox_shader::resolution, &resolution, SHADER_ATTRIB_VEC2);
	SetShaderValue(skybox_shader::shader, skybox_shader::fov, &fov, SHADER_ATTRIB_FLOAT);
	SetShaderValueV(skybox_shader::shader, skybox_shader::palette, &skybox_shader::palette_vec, SHADER_ATTRIB_VEC4, 4);
	SetShaderValueMatrix(skybox_shader::shader, skybox_shader::matCamera, camera_matrix);

    Texture2D starmap = assets::GetTexture("resources/textures/starmap_4k.jpg");
    SetShaderValueTexture(skybox_shader::shader, skybox_shader::starMap, starmap);

    DrawTexture(starmap, INT32_MAX, INT32_MAX, WHITE);
    //rlDisableDepthTest();
    //rlDisableDepthMask();
    BeginShaderMode(skybox_shader::shader);

    _RenderQuad(Palette::ui_alt);

    EndShaderMode();
    //rlEnableDepthMask();
    //rlEnableDepthTest();
}

namespace postprocessing_shader {
    Shader shader;
    int depthMap = -1;
    
    void Load() {
        LOAD_SHADER(postprocessing_shader)
        LOAD_SHADER_UNIFORM(postprocessing_shader, depthMap);
    }
}

void RenderDeferred(RenderTexture render_target) {
    Rectangle screen_rect = {
        0, 0, GetScreenWidth(), -GetScreenHeight()
    };

    if (!GetSettingBool("skip_postprocessing", false)) {
        RELOAD_IF_NECESSARY(postprocessing_shader)

        BeginShaderMode(postprocessing_shader::shader);
        SetShaderValueTexture(postprocessing_shader::shader, postprocessing_shader::depthMap, render_target.depth);
    }

    DrawTextureRec(render_target.texture, screen_rect, Vector2Zero(), WHITE);
    EndShaderMode();
}

bool ShaderNeedReload(Shader shader) {
    return !IsShaderReady(shader);
}

AtlasPos::AtlasPos(int x, int y) {
    this->x = x;
    this->y = y;
}

Rectangle AtlasPos::GetRect(int size) const {
    Rectangle res;
    res.x = x * size;
    res.y = y * size;
    res.width = size;
    res.height = size;
    return res;
}
