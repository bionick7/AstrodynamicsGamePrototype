#include "render_utils.hpp"

#include "astro.hpp"
#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "ui.hpp"
#include "debug_console.hpp"

#include "rlgl.h"

#define ORBIT_BUFFER_SIZE 1024

namespace default_buffers {
    int orbit_vao = -1;
    int orbit_vbo = -1;

    void ReloadIfNecaissary() {
        // Not expected to unload / reload
        if (orbit_vao < 0 || orbit_vbo < 0) {
            float* float_array = new float[ORBIT_BUFFER_SIZE];

            for (int i=0; i < ORBIT_BUFFER_SIZE; i++) {
                float_array[i] = i / (float)ORBIT_BUFFER_SIZE;
            }

            orbit_vao = rlLoadVertexArray();
            rlEnableVertexArray(orbit_vao);

            orbit_vbo = rlLoadVertexBuffer(float_array, ORBIT_BUFFER_SIZE*sizeof(float), false);
            rlSetVertexAttribute(0, 1, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(0);
            rlDisableVertexArray();
            delete[] float_array;
        }
    }
};

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

void _ColorToFloat4Buffer(float buffer[], Color color) {
    buffer[0] = (float)color.r / 255.0f;
    buffer[1] = (float)color.g / 255.0f;
    buffer[2] = (float)color.b / 255.0f;
    buffer[3] = (float)color.a / 255.0f;
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

    void Load() {
        LOAD_SHADER(ui_shader)
        LOAD_SHADER_UNIFORM(ui_shader, depth)
    }
}

void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer) {
    RELOAD_IF_NECAISSARY(sdf_shader)
    BeginShaderMode(sdf_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float bg_color_4[4];
    _ColorToFloat4Buffer(bg_color_4, Palette::bg);
    SetShaderValue(sdf_shader::shader, sdf_shader::background_color, bg_color_4, SHADER_UNIFORM_VEC4);
    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
    DrawTexturePro(texture, source, dest, origin, rotation, tint);
    EndShaderMode();
}

void BeginRenderSDFInUIMode(uint8_t z_layer, Color background) {
    RELOAD_IF_NECAISSARY(sdf_shader)
    BeginShaderMode(sdf_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float bg_color_4[4];
    _ColorToFloat4Buffer(bg_color_4, background);
    SetShaderValue(sdf_shader::shader, sdf_shader::background_color, bg_color_4, SHADER_UNIFORM_VEC4);
    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
}

void BeginRenderInUILayer(uint8_t z_layer) {
    RELOAD_IF_NECAISSARY(ui_shader)
    BeginShaderMode(ui_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    SetShaderValue(ui_shader::shader, ui_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
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

    void Load() {
        LOAD_SHADER(wireframe_shader)
        LOAD_SHADER_UNIFORM(wireframe_shader, render_mode)
        LOAD_SHADER_UNIFORM(wireframe_shader, color)
        LOAD_SHADER_UNIFORM(wireframe_shader, mvp)
        LOAD_SHADER_UNIFORM(wireframe_shader, time)
        LOAD_SHADER_UNIFORM(wireframe_shader, depth)
    }
}

void RenderWireframeMesh(WireframeMesh mesh, Matrix transform, Color background, Color foreground) {
    RELOAD_IF_NECAISSARY(wireframe_shader)

    static float bg_color[4];
    static float fg_color[4];
    _ColorToFloat4Buffer(bg_color, background);
    _ColorToFloat4Buffer(fg_color, foreground);

    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();
    Matrix matModel = MatrixMultiply(transform, rlGetMatrixTransform());
    Matrix matModelView = MatrixMultiply(matModel, matView);
    Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);

    int render_mode_lines = 0;
    int render_mode_faces = 3;
    float default_depth = -1.0;

    rlEnableShader(wireframe_shader::shader.id);
    rlSetUniformMatrix(wireframe_shader::mvp, matModelViewProjection);
    rlSetUniform(wireframe_shader::depth, &default_depth, RL_SHADER_UNIFORM_FLOAT, 1);

    rlSetUniform(wireframe_shader::render_mode, &render_mode_faces, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, bg_color, RL_SHADER_UNIFORM_VEC4, 1);
    rlDisableBackfaceCulling();

    rlEnableVertexArray(mesh.vao_triangles);
    rlDrawVertexArrayElements(0, mesh.triangle_count*3, 0);
    rlDisableVertexArray();

    rlSetUniform(wireframe_shader::render_mode, &render_mode_lines, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, fg_color, RL_SHADER_UNIFORM_VEC4, 1);

    rlEnableVertexArray(mesh.vao_lines);
    rlDrawVertexLineArrayElements(0, mesh.line_count*2, 0);
    rlDisableVertexArray();
    
    rlDisableShader();
}

void RenderWireframeMesh2DEx(WireframeMesh mesh, Vector2 origin, float scale, 
                             Color background, Color foreground, uint8_t z_layer) {
    RELOAD_IF_NECAISSARY(wireframe_shader)

    static float bg_color[4];
    static float fg_color[4];
    _ColorToFloat4Buffer(bg_color, background);
    _ColorToFloat4Buffer(fg_color, foreground);

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
    float time = GetTime();
    float z_layer_f = 1.0f - z_layer / 256.0f;
    float z_layer_f_lines = 1.0f - (z_layer + 1) / 256.0f;

    rlEnableShader(wireframe_shader::shader.id);

    rlSetUniform(wireframe_shader::time, &time, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniformMatrix(wireframe_shader::mvp, model2ndc_matrix);

    rlSetUniform(wireframe_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_faces, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, bg_color, RL_SHADER_UNIFORM_VEC4, 1);
    rlDisableBackfaceCulling();

    rlEnableVertexArray(mesh.vao_triangles);
    rlDrawVertexArrayElements(0, mesh.triangle_count*3, 0);
    rlDisableVertexArray();

    rlSetUniform(wireframe_shader::depth, &z_layer_f_lines, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(wireframe_shader::render_mode, &render_mode_lines, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(wireframe_shader::color, fg_color, RL_SHADER_UNIFORM_VEC4, 1);

    rlEnableVertexArray(mesh.vao_lines);
    rlDrawVertexLineArrayElements(0, mesh.line_count*2, 0);
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

namespace orbit_shader {
    Shader shader;
    int semi_latus_rectum = -1;
    int eccentricity = -1;
    int current_anomaly = -1;
    int focal_bound_start = -1;
    int focal_range = -1;

    int color = -1;
    int render_mode = -1;

    int orbit_transform = -1;
    int mvp = -1;

    void Load() {
        LOAD_SHADER(orbit_shader)
        LOAD_SHADER_UNIFORM(orbit_shader, semi_latus_rectum)
        LOAD_SHADER_UNIFORM(orbit_shader, eccentricity)
        LOAD_SHADER_UNIFORM(orbit_shader, current_anomaly)
        LOAD_SHADER_UNIFORM(orbit_shader, focal_bound_start)
        LOAD_SHADER_UNIFORM(orbit_shader, focal_range)

        LOAD_SHADER_UNIFORM(orbit_shader, color)
        LOAD_SHADER_UNIFORM(orbit_shader, render_mode)

        LOAD_SHADER_UNIFORM(orbit_shader, mvp)
    }
}

void RenderOrbit(const OrbitSegment *segment, orbit_render_mode::T render_mode, Color color) {
    RELOAD_IF_NECAISSARY(orbit_shader)
    default_buffers::ReloadIfNecaissary();

    const Orbit* orbit = segment->orbit;

    float p = (float) (orbit->sma * (1 - orbit->ecc*orbit->ecc) / GameCamera::space_scale);
    float ecc = (float) orbit->ecc;
    Vector3 mat_x = (Vector3) orbit->periapsis_dir;
    Vector3 mat_y = (Vector3) orbit->normal;
    Vector3 mat_z = Vector3CrossProduct(mat_y, mat_x);
    Matrix matModel = MatrixFromColumns(mat_x, mat_y, mat_z);
    Matrix matModelView = MatrixMultiply(matModel, rlGetMatrixModelview());
    Matrix matModelViewProjection = MatrixMultiply(matModelView, rlGetMatrixProjection());

    float current_focal_anomaly = orbit->GetPosition(GlobalGetNow()).θ;

    float focal_bound1 = 0;
    float focal_range = 0;
    if (segment->is_full_circle) {
        focal_bound1 = 0;
        focal_range = 2*PI;
    } else {
        focal_bound1 = segment->bound1.θ;
        float focal_bound2;
        focal_bound2 = segment->bound2.θ;
        if (focal_bound2 < focal_bound1) focal_bound2 += 2*PI;
        focal_range = focal_bound2 - focal_bound1;
    }

    int render_mode_int = render_mode;
    float color4[4];
    _ColorToFloat4Buffer(color4, color);

    rlEnableShader(orbit_shader::shader.id);
    rlSetUniform(orbit_shader::semi_latus_rectum, &p, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(orbit_shader::eccentricity, &ecc, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(orbit_shader::current_anomaly, &current_focal_anomaly, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(orbit_shader::focal_bound_start, &focal_bound1, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(orbit_shader::focal_range, &focal_range, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(orbit_shader::color, color4, RL_SHADER_UNIFORM_VEC4, 1);

    rlSetUniform(orbit_shader::render_mode, &render_mode_int, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniformMatrix(orbit_shader::mvp, matModelViewProjection);


    // Maybe keep around for performence compairison
    //const int point_count = 1024;
    //BeginShaderMode(orbit_shader::shader);
    //rlBegin(RL_LINES);
    //float delta = 1.0 / (float) point_count;
    //rlVertex3f(0.0, 0.0, 1.0);
    //for (int j=0; j < point_count; j++) {
    //    float focal = j * delta;
    //    rlVertex3f(focal, sin(focal), cos(focal));
    //    rlVertex3f(focal, sin(focal), cos(focal));
    //}
    //rlVertex3f(1.0, 0.0, 1.0);
    //rlEnd();
    //EndShaderMode();
    
    rlEnableVertexArray(default_buffers::orbit_vao);
    rlDrawVertexLineStripArray(0, ORBIT_BUFFER_SIZE);
    rlDisableVertexArray();
    
    rlDisableShader();
}

namespace planet_shader {
    Shader shader;

    int transform = -1;

    int edge = -1;
    int space = -1;
    int radius = -1;
    int screenWidth = -1;

    int centerPos = -1;
    int cameraPos = -1;
    int normal = -1;
    int rimColor = -1;
    int fillColor = -1;
    
    void Load() {
        LOAD_SHADER(planet_shader)
        LOAD_SHADER_UNIFORM(planet_shader, transform)
        LOAD_SHADER_UNIFORM(planet_shader, edge)
        LOAD_SHADER_UNIFORM(planet_shader, space)
        LOAD_SHADER_UNIFORM(planet_shader, radius)
        LOAD_SHADER_UNIFORM(planet_shader, screenWidth)
        LOAD_SHADER_UNIFORM(planet_shader, centerPos)
        LOAD_SHADER_UNIFORM(planet_shader, cameraPos)
        LOAD_SHADER_UNIFORM(planet_shader, normal)
        LOAD_SHADER_UNIFORM(planet_shader, rimColor)
        LOAD_SHADER_UNIFORM(planet_shader, fillColor)
    }
}

void RenderPerfectSphere(DVector3 world_position, double radius, Color color) {
    RELOAD_IF_NECAISSARY(planet_shader)

    Vector3 render_position = GameCamera::WorldToRender(world_position);
    float render_radius = GameCamera::WorldToRender(radius);
    Vector3 render_camera_pos = GetCamera()->rl_camera.position;
    //Vector3 world_camera_dir = Vector3Subtract(GetCamera()->rl_camera.target, GetCamera()->rl_camera.position);

    float rim_color4[4];
    _ColorToFloat4Buffer(rim_color4, color);
    SetShaderValue(planet_shader::shader, planet_shader::rimColor, rim_color4, SHADER_UNIFORM_VEC4);
    float fill_color4[4];
    _ColorToFloat4Buffer(fill_color4, Palette::bg);
    SetShaderValue(planet_shader::shader, planet_shader::fillColor, fill_color4, SHADER_UNIFORM_VEC4);

	Vector3 mat_z = Vector3Subtract(GetCamera()->rl_camera.position, render_position);  // In world space
    float camera_distance = Vector3Length(mat_z);
	Vector3 mat_y = { 0.0f, 1.0f, 0.0f };  // Chosen arbitrarily

    Vector3OrthoNormalize(&mat_z, &mat_y);
    Vector3 mat_x = Vector3CrossProduct(mat_y, mat_z);

	float offset = render_radius * render_radius / camera_distance;  // Offset from midpoint to plane in render-world coordinate system
	float h = sqrt(render_radius * render_radius - offset * offset);  // plane scale in render-world coordinate system
	//float h = render_radius * sqrt(1 - 1/camera_distance);  // plane scale in render-world coordinate system

	Matrix new_model_matrix = MatrixFromColumns(
        Vector3Scale(mat_x, h),
        Vector3Scale(mat_y, h),
        Vector3Scale(mat_z, h)
    );	// Model to view such that model faces you

	new_model_matrix.m12 = render_position.x + mat_z.x * offset;
	new_model_matrix.m13 = render_position.y + mat_z.y * offset;
	new_model_matrix.m14 = render_position.z + mat_z.z * offset;

    //DebugDrawTransform(new_model_matrix);

    //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_x));
    //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_y));
    //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_z));

	// Set parameters
    float line_thickness = 2.0f;
    float screen_width = GetScreenWidth();

	SetShaderValue(planet_shader::shader, planet_shader::space, &line_thickness, SHADER_UNIFORM_FLOAT);
	SetShaderValue(planet_shader::shader, planet_shader::edge, &line_thickness, SHADER_UNIFORM_FLOAT);
	SetShaderValue(planet_shader::shader, planet_shader::screenWidth, &screen_width, SHADER_UNIFORM_FLOAT);
	SetShaderValue(planet_shader::shader, planet_shader::radius, &render_radius, SHADER_UNIFORM_FLOAT);
	SetShaderValue(planet_shader::shader, planet_shader::cameraPos, &render_camera_pos, SHADER_UNIFORM_VEC3);
	SetShaderValue(planet_shader::shader, planet_shader::centerPos, &render_position, SHADER_UNIFORM_VEC3);
	SetShaderValue(planet_shader::shader, planet_shader::normal, &mat_z, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(planet_shader::shader, planet_shader::transform, new_model_matrix);

    BeginShaderMode(planet_shader::shader);
    _RenderQuad(color);
    EndShaderMode();

    //DrawSphereWires(render_position, render_radius, 16, 16, WHITE);
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
    RELOAD_IF_NECAISSARY(rings_shader)
    
    float inner_rad_float = min_rad / max_rad;
	SetShaderValue(rings_shader::shader, rings_shader::innerRad, &inner_rad_float, SHADER_ATTRIB_FLOAT);
    
    float bg_color4[4];
    _ColorToFloat4Buffer(bg_color4, Palette::bg);
	SetShaderValue(rings_shader::shader, rings_shader::bgColor, &bg_color4, SHADER_ATTRIB_VEC4);

    rlPushMatrix();
    // scale -> rotate -> translate
    float render_scale = GameCamera::WorldToRender(max_rad);
    rlScalef(render_scale, render_scale, render_scale);
    rlRotatef(90, 1, 0, 0);
    rlTranslatef(0, 0, 0.001);

    BeginShaderMode(rings_shader::shader);
    _RenderDoubleQuad(color);
    EndShaderMode();

    rlPopMatrix();
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

        
        _ColorToFloat4Buffer(&palette_vec[4*0], Palette::bg);
        _ColorToFloat4Buffer(&palette_vec[4*1], Palette::ui_dark);
        _ColorToFloat4Buffer(&palette_vec[4*3], Palette::ui_alt);
        _ColorToFloat4Buffer(&palette_vec[4*2], Palette::ui_alt);
    }
}

void RenderSkyBox() {
    RELOAD_IF_NECAISSARY(skybox_shader)

    Vector2 resolution = { GetScreenWidth(), GetScreenHeight() };
    
    float fov = GetCamera()->rl_camera.fovy * DEG2RAD;
    //Matrix camera_matrix = MatrixInvert(rlGetMatrixModelview());

    Vector3 mat_z = Vector3Subtract(GetCamera()->rl_camera.position, GetCamera()->rl_camera.target);  // In world space
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
        RELOAD_IF_NECAISSARY(postprocessing_shader)

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
