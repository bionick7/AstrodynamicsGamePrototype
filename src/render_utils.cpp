#include "render_utils.hpp"

#include "astro.hpp"
#include "coordinate_transform.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "constants.hpp"
#include "utils.hpp"

#include "rlgl.h"


void EsureShaderReady(Shader* shader, const char* name) {
    if (IsShaderReady(*shader)) {
        return;
    }
    StringBuilder sb;
    const char* vertex_shader_path = sb.Add("resources/shaders/").Add(name).Add(".vs").c_str;
    char* fragment_shader_path = new char[strlen(vertex_shader_path) + 1];
    strcpy(fragment_shader_path, vertex_shader_path);
    fragment_shader_path[strlen(vertex_shader_path)-2] = 'f';  // swap out 'v' with 'f'
    *shader = LoadShader(vertex_shader_path, fragment_shader_path);
}

void ColorToFloat4Buffer(float buffer[], Color color) {
    buffer[0] = (float)color.r / 255.0f;
    buffer[1] = (float)color.g / 255.0f;
    buffer[2] = (float)color.b / 255.0f;
    buffer[3] = (float)color.a / 255.0f;
}

#define LOAD_SHADER(shader_name) shader_name::shader = LoadShader("resources/shaders/"#shader_name".vs", "resources/shaders/"#shader_name".fs");
#define LOAD_SHADER_UNIFORM(shader_name, uniform_name) shader_name::uniform_name = GetShaderLocation(shader_name::shader, #uniform_name);

namespace orbit_shader {
    Shader shader;
    int semi_latus_rectum = -1;
    int eccentricity = -1;
    int orbit_transform = -1;
    int current_anomaly = -1;

    int color = -1;
}

void RenderOrbit(const OrbitSegment* segment, int point_count, Color color) {
    if (!IsShaderReady(orbit_shader::shader)) {
        LOAD_SHADER(orbit_shader)
        LOAD_SHADER_UNIFORM(orbit_shader, semi_latus_rectum)
        LOAD_SHADER_UNIFORM(orbit_shader, eccentricity)
        LOAD_SHADER_UNIFORM(orbit_shader, orbit_transform)
        LOAD_SHADER_UNIFORM(orbit_shader, current_anomaly)
        LOAD_SHADER_UNIFORM(orbit_shader, color)
    }
    const Orbit* orbit = segment->orbit;

    //double p = planet_array[i].orbit.sma * (1 - planet_array[i].orbit.ecc*planet_array[i].orbit.ecc);
    //for (int j=0; j < point_count; j++) {
    //    double θ1 = j / (double) point_count * 2*PI;
    //    double θ2 = (j + 1) / (double) point_count * 2*PI;
    //    double r1 = p / (1 + planet_array[i].orbit.ecc*cos(θ1));
    //    double r2 = p / (1 + planet_array[i].orbit.ecc*cos(θ2));
    //    DVector3 dir1 = planet_array[i].orbit.periapsis_dir.RotatedByAxisAngle(planet_array[i].orbit.normal, θ1);
    //    DVector3 dir2 = planet_array[i].orbit.periapsis_dir.RotatedByAxisAngle(planet_array[i].orbit.normal, θ2);
    //    DVector3 pos1 = r1 / GameCamera::space_scale * dir1;
    //    DVector3 pos2 = r2 / GameCamera::space_scale * dir2;
    //    rlColor4f(1,1,1,1);
    //    rlVertex3f(pos1.x, pos1.y, pos1.z);
    //    rlVertex3f(pos2.x, pos2.y, pos2.z);
    //}

    //double p = planet_array[i].orbit.sma * (1 - planet_array[i].orbit.ecc*planet_array[i].orbit.ecc);
    //Vector3 v_ecc =  (Vector3) (planet_array[i].orbit.periapsis_dir * planet_array[i].orbit.ecc);
    //Vector3 v_norm = (Vector3) (planet_array[i].orbit.normal * p / GameCamera::space_scale);
    //for (int j=0; j < point_count; j++) {
    //    rlColor4f(v_norm.x, v_norm.y, v_norm.z, v_ecc.z);  // akwardly need to swap v_ecc.z and anomaly
    //    rlVertex3f(v_ecc.x, v_ecc.y, ((j + 1) % point_count) / (float)point_count * 2*PI);
    //    rlVertex3f(v_ecc.x, v_ecc.y,                       j / (float)point_count * 2*PI);
    //}

    float p = (float) (orbit->sma * (1 - orbit->ecc*orbit->ecc) / GameCamera::space_scale);
    float ecc = (float) orbit->ecc;
    Vector3 mat_x = (Vector3) orbit->periapsis_dir;
    Vector3 mat_y = (Vector3) orbit->normal;
    Vector3 mat_z = Vector3CrossProduct(mat_x, mat_y);

    float current_focal_anomaly = orbit->GetPosition(GlobalGetNow()).θ;

    SetShaderValue(orbit_shader::shader, orbit_shader::semi_latus_rectum, &p, SHADER_UNIFORM_FLOAT);
    SetShaderValue(orbit_shader::shader, orbit_shader::eccentricity, &ecc, SHADER_UNIFORM_FLOAT);
    SetShaderValue(orbit_shader::shader, orbit_shader::current_anomaly, &current_focal_anomaly, SHADER_UNIFORM_FLOAT);
    SetShaderValueMatrix(orbit_shader::shader, orbit_shader::orbit_transform, MatrixFromColumns(mat_x, mat_y, mat_z));

    float color4[4];
    ColorToFloat4Buffer(color4, color);
    SetShaderValue(orbit_shader::shader, orbit_shader::color, color4, SHADER_UNIFORM_VEC4);

    BeginShaderMode(orbit_shader::shader);
    rlBegin(RL_LINES);
    if (segment->is_full_circle) {
        float delta = 2*PI / (float) point_count;
        for (int j=0; j < point_count; j++) {
            rlVertex3f(j * delta, 0, 0);
            rlVertex3f((j + 1) * delta, 0, 0);
        }
    } else {
        float delta = (segment->bound2.θ - segment->bound1.θ) / (float) point_count;
        for (int j=0; j < point_count; j++) {
            rlVertex3f(segment->bound1.θ + j * delta, 0, 0);
            rlVertex3f(segment->bound1.θ + (j + 1) * delta, 0, 0);
        }
    }

    rlEnd();
    EndShaderMode();
}


namespace planet_shader {
    Shader shader;
    int transform;

    int edge;
    int space;
    int radius;
    int screenWidth;

    int cameraPos;
    int normal;
    int rim_color;
    int fill_color;
}

void RenderPerfectSphere(DVector3 world_position, double radius, Color color) {

    if (!IsShaderReady(planet_shader::shader)) {
        LOAD_SHADER(planet_shader)
        LOAD_SHADER_UNIFORM(planet_shader, transform)
        LOAD_SHADER_UNIFORM(planet_shader, edge)
        LOAD_SHADER_UNIFORM(planet_shader, space)
        LOAD_SHADER_UNIFORM(planet_shader, radius)
        LOAD_SHADER_UNIFORM(planet_shader, screenWidth)
        LOAD_SHADER_UNIFORM(planet_shader, cameraPos)
        LOAD_SHADER_UNIFORM(planet_shader, normal)
        LOAD_SHADER_UNIFORM(planet_shader, rim_color)
        LOAD_SHADER_UNIFORM(planet_shader, fill_color)
    }

    Vector3 render_position = (Vector3) (world_position / GameCamera::space_scale);
    float render_radius = radius / GameCamera::space_scale;
    Vector3 world_camera_pos = GetCamera()->rl_camera.position;
    Vector3 world_camera_dir = Vector3Subtract(GetCamera()->rl_camera.target, GetCamera()->rl_camera.position);

    float rim_color4[4];
    ColorToFloat4Buffer(rim_color4, color);
    SetShaderValue(planet_shader::shader, planet_shader::rim_color, rim_color4, SHADER_UNIFORM_VEC4);
    float fill_color4[4];
    ColorToFloat4Buffer(fill_color4, Palette::bg);
    SetShaderValue(planet_shader::shader, planet_shader::fill_color, fill_color4, SHADER_UNIFORM_VEC4);

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
	SetShaderValue(planet_shader::shader, planet_shader::cameraPos, &world_camera_pos, SHADER_UNIFORM_VEC3);
	SetShaderValue(planet_shader::shader, planet_shader::normal, &mat_z, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(planet_shader::shader, planet_shader::transform, new_model_matrix);

    BeginShaderMode(planet_shader::shader);
    DrawTriangle3D(
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
    color);
    DrawTriangle3D(
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    color);
    EndShaderMode();
}