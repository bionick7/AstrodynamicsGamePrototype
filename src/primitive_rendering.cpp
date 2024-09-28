#include "primitive_rendering.hpp"

#include "astro.hpp"
#include "utils.hpp"
#include "global_state.hpp"
#include "coordinate_transform.hpp"

#include "rlgl.h"

#define ORBIT_RESOLUTION 64
#define ORBIT_BUFFER_SIZE ORBIT_RESOLUTION * 6 * 2


ConicRenderInfo ConicRenderInfo::FromOrbitSegment(const OrbitSegment *segment, 
                                                  orbit_render_mode::T render_mode, Color color) {
    ConicRenderInfo res;

    const Orbit* orbit = segment->orbit;

    res.center = Vector3Zero();
    res.semi_latus_rectum = (float) (orbit->sma * (1 - orbit->ecc*orbit->ecc) / GameCamera::space_scale);
    res.eccentricity = (float) orbit->ecc;
    Vector3 mat_x = (Vector3) orbit->periapsis_dir;
    Vector3 mat_y = (Vector3) orbit->normal;
    Vector3 mat_z = Vector3CrossProduct(mat_y, mat_x);
    res.orientation = MatrixFromColumns(mat_x, mat_y, mat_z);
    res.anomaly = orbit->GetPosition(GlobalGetNow()).θ;

    float current_focal_anomaly = orbit->GetPosition(GlobalGetNow()).θ;

    if (segment->is_full_circle) {
        res.focal_bound = 0;
        res.focal_range = 2*PI;
    } else {
        res.focal_bound = segment->bound1.θ;
        float focal_bound2;
        focal_bound2 = segment->bound2.θ;
        if (focal_bound2 < res.focal_bound) focal_bound2 += 2*PI;
        res.focal_range = focal_bound2 - res.focal_bound;
    }

    res.render_mode = render_mode;
    res.color = color;

    return res;
}

ConicRenderInfo ConicRenderInfo::FromOrbit(const Orbit *orbit, timemath::Time time,
                                           orbit_render_mode::T render_mode, Color color) {
    ConicRenderInfo res;

    res.center = Vector3Zero();
    res.semi_latus_rectum = (float) (orbit->sma * (1 - orbit->ecc*orbit->ecc) / GameCamera::space_scale);
    res.eccentricity = (float) orbit->ecc;
    Vector3 mat_x = (Vector3) orbit->periapsis_dir;
    Vector3 mat_y = (Vector3) orbit->normal;
    Vector3 mat_z = Vector3CrossProduct(mat_y, mat_x);
    res.orientation = MatrixFromColumns(mat_x, mat_y, mat_z);

    res.focal_bound = 0;
    res.focal_range = 2*PI;
    res.anomaly = orbit->GetPosition(time).θ;

    res.render_mode = render_mode;
    res.color = color;

    return res;
}


ConicRenderInfo ConicRenderInfo::FromCircle(DVector3 world_position, Matrix orientation, double radius, 
                                            orbit_render_mode::T render_mode, Color color) {
    ConicRenderInfo res;

    res.center = GameCamera::WorldToRender(world_position);
    res.semi_latus_rectum = GameCamera::WorldToRender(radius);
    res.eccentricity = 0.0f;
    res.orientation = orientation;

    res.focal_bound = 0;
    res.focal_range = 2*PI;
    res.anomaly = 0;

    res.render_mode = render_mode;
    res.color = color;

    return res;
}

SphereRenderInfo SphereRenderInfo::FromWorldPos(DVector3 center, double radius, Color color) {
    SphereRenderInfo res;
    res.center = GameCamera::WorldToRender(center);
    res.radius = GameCamera::WorldToRender(radius);
    res.color = color;
    return res;
}

namespace orbit_render_buffers {
    int vao = -1;
    int vbo = -1;

    int buffer_size = 0;

    float* semi_latus_recta = NULL;
    float* eccentricities = NULL;
    float* current_anomalies = NULL;
    float* focal_bounds_start = NULL;
    float* focal_ranges = NULL;
    float* colors = NULL;
    Vector3* offsets = NULL;
    int* render_modes = NULL;
    Matrix* mvps = NULL;

    void ReloadBuffer(int orbits_count) {
        if (orbits_count <= buffer_size) {
            // Not expected to unload / reload
            return;
        }
        if (buffer_size != 0) {
            // Not the first time, this is called
            rlUnloadVertexArray(vao);
            rlUnloadVertexBuffer(vbo);

            delete[] semi_latus_recta;
            delete[] eccentricities;
            delete[] current_anomalies;
            delete[] focal_bounds_start;
            delete[] focal_ranges;
            delete[] colors;
            delete[] offsets;
            delete[] render_modes;
            delete[] mvps;
        }

        // Regenerate vertex buffer
        float* float_array = new float[ORBIT_BUFFER_SIZE * orbits_count];

        int it = 0;
        for (int i=0; i < ORBIT_RESOLUTION * orbits_count; i++) {
            float progress = (i % ORBIT_RESOLUTION) / (float)ORBIT_RESOLUTION;
            float next_progress = progress + 1.0f / (float)ORBIT_RESOLUTION;
            float_array[it++] = progress;
            float_array[it++] = -1;
            float_array[it++] = progress;
            float_array[it++] = 1;
            float_array[it++] = next_progress;
            float_array[it++] = 1;
            
            float_array[it++] = next_progress;
            float_array[it++] = 1;
            float_array[it++] = next_progress;
            float_array[it++] = -1;
            float_array[it++] = progress;
            float_array[it++] = -1;
        }

        vao = rlLoadVertexArray();
        rlEnableVertexArray(vao);

        vbo = rlLoadVertexBuffer(float_array, orbits_count * ORBIT_BUFFER_SIZE*sizeof(float), false);
        rlSetVertexAttribute(0, 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(0);
        rlDisableVertexArray();
        delete[] float_array;

        buffer_size = orbits_count;

        // Reinitialize arrays
        semi_latus_recta = new float[orbits_count];
        eccentricities = new float[orbits_count];
        current_anomalies = new float[orbits_count];
        focal_bounds_start = new float[orbits_count];
        focal_ranges = new float[orbits_count];
        colors = new float[orbits_count*4];
        offsets = new Vector3[orbits_count];
        render_modes = new int[orbits_count];
        mvps = new Matrix[orbits_count];
    }
};

namespace orbit_shader {
    Shader shader;
    int semi_latus_rectum = -1;
    int eccentricity = -1;
    int current_anomaly = -1;
    int focal_bound_start = -1;
    int focal_range = -1;
    int offset = -1;

    int color = -1;
    int render_mode = -1;
    int window_size = -1;

    int orbit_transform = -1;
    int mvp = -1;

    void Load() {
        LOAD_SHADER(orbit_shader)
        LOAD_SHADER_UNIFORM(orbit_shader, semi_latus_rectum)
        LOAD_SHADER_UNIFORM(orbit_shader, eccentricity)
        LOAD_SHADER_UNIFORM(orbit_shader, current_anomaly)
        LOAD_SHADER_UNIFORM(orbit_shader, focal_bound_start)
        LOAD_SHADER_UNIFORM(orbit_shader, focal_range)
        LOAD_SHADER_UNIFORM(orbit_shader, offset)

        LOAD_SHADER_UNIFORM(orbit_shader, color)
        LOAD_SHADER_UNIFORM(orbit_shader, render_mode)
        LOAD_SHADER_UNIFORM(orbit_shader, window_size)

        LOAD_SHADER_UNIFORM(orbit_shader, mvp)
    }
}

void RenderOrbits(const ConicRenderInfo conics[], int number) {
    RELOAD_IF_NECESSARY(orbit_shader)
    orbit_render_buffers::ReloadBuffer(number);

    for (int i=0; i < number; i++) {
        orbit_render_buffers::semi_latus_recta[i] = conics[i].semi_latus_rectum;
        orbit_render_buffers::eccentricities[i] = conics[i].eccentricity;
        orbit_render_buffers::focal_bounds_start[i] = conics[i].focal_bound;
        orbit_render_buffers::focal_ranges[i] = conics[i].focal_range;
        orbit_render_buffers::current_anomalies[i] = conics[i].anomaly;

        Matrix matModelView = MatrixMultiply(conics[i].orientation, rlGetMatrixModelview());
        orbit_render_buffers::mvps[i] = MatrixMultiply(matModelView, rlGetMatrixProjection());

        orbit_render_buffers::render_modes[i] = conics[i].render_mode;
        ColorToFloatBuffer(&orbit_render_buffers::colors[i*4], conics[i].color);
        orbit_render_buffers::offsets[i] = conics[i].center;
    }

    float window_size[2] = { GetScreenWidth(), GetScreenHeight() };

    rlEnableShader(orbit_shader::shader.id);
    rlSetUniform(orbit_shader::semi_latus_rectum, orbit_render_buffers::semi_latus_recta, RL_SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(orbit_shader::eccentricity, orbit_render_buffers::eccentricities, RL_SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(orbit_shader::current_anomaly, orbit_render_buffers::current_anomalies, RL_SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(orbit_shader::focal_bound_start, orbit_render_buffers::focal_bounds_start, RL_SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(orbit_shader::focal_range, orbit_render_buffers::focal_ranges, RL_SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(orbit_shader::color, orbit_render_buffers::colors, RL_SHADER_UNIFORM_VEC4, number);
    rlSetUniform(orbit_shader::offset, orbit_render_buffers::offsets, RL_SHADER_UNIFORM_VEC3, number);
    rlSetUniform(orbit_shader::window_size, window_size, RL_SHADER_UNIFORM_VEC2, 1);

    rlSetUniform(orbit_shader::render_mode, orbit_render_buffers::render_modes, RL_SHADER_UNIFORM_INT, number);
    rlSetUniformMatrixArray(orbit_shader::mvp, orbit_render_buffers::mvps, number);

    rlEnableVertexArray(orbit_render_buffers::vao);
    rlDrawVertexArray(0, ORBIT_BUFFER_SIZE * number);
    //rlDrawVertexPrimitiveArray(RL_LINES, 0, ORBIT_BUFFER_SIZE);
    rlDisableVertexArray();
    
    rlDisableShader();
}

namespace planet_render_buffers {
    int vao = -1;
    int vbo = -1;

    int buffer_size = 0;

    float* radius = NULL;
    Vector3* centerPos = NULL;
    Vector3* normal = NULL;
    float* color = NULL;
    Matrix* transform = NULL;

    const int VALS_PER_QUAD = 18;

    void Reload(int sphere_count) {

        if (sphere_count <= buffer_size) {
            // Not expected to unload / reload
            return;
        }
        if (buffer_size != 0) {
            // Not the first time
            rlUnloadVertexArray(vao);
            rlUnloadVertexBuffer(vbo);

            delete[] radius;
            delete[] centerPos;
            delete[] normal;
            delete[] color;
            delete[] transform;
        }
        
        float* float_array = new float[VALS_PER_QUAD * sphere_count];

        static float quad_vertex_array[VALS_PER_QUAD] = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
        };

        // Regenerate vertex buffer
        for (int i=0; i < sphere_count; i++) {
            memcpy(&float_array[i*VALS_PER_QUAD], quad_vertex_array, VALS_PER_QUAD * sizeof(float));
        }

        vao = rlLoadVertexArray();
        rlEnableVertexArray(vao);

        vbo = rlLoadVertexBuffer(float_array, sphere_count * VALS_PER_QUAD * sizeof(float), false);
        rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(0);
        rlDisableVertexArray();
        delete[] float_array;
        
        buffer_size = sphere_count;

        // Reinitialize arrays
        radius    = new float[sphere_count];
        centerPos = new Vector3[sphere_count];
        normal    = new Vector3[sphere_count];
        color     = new float[sphere_count*4];
        transform = new Matrix[sphere_count];
    }
}

namespace planet_shader {
    Shader shader;


    int edge = -1;
    int space = -1;
    int radius = -1;
    int screenWidth = -1;
    int cameraPos = -1;

    int centerPos = -1;
    int normal = -1;
    int rimColor = -1;
    int fillColor = -1;

    int mvp = -1;
    int transform = -1;
    
    void Load() {
        LOAD_SHADER(planet_shader)
        LOAD_SHADER_UNIFORM(planet_shader, edge)
        LOAD_SHADER_UNIFORM(planet_shader, space)
        LOAD_SHADER_UNIFORM(planet_shader, radius)
        LOAD_SHADER_UNIFORM(planet_shader, screenWidth)
        LOAD_SHADER_UNIFORM(planet_shader, cameraPos)

        LOAD_SHADER_UNIFORM(planet_shader, centerPos)
        LOAD_SHADER_UNIFORM(planet_shader, normal)
        LOAD_SHADER_UNIFORM(planet_shader, rimColor)
        LOAD_SHADER_UNIFORM(planet_shader, fillColor)

        LOAD_SHADER_UNIFORM(planet_shader, mvp)
        LOAD_SHADER_UNIFORM(planet_shader, transform)
    }
}

void RenderPerfectSpheres(const SphereRenderInfo spheres[], int number) {
    RELOAD_IF_NECESSARY(planet_shader)
    planet_render_buffers::Reload(number);

    Vector3 render_camera_pos = GetCamera()->rl_camera.position;

    for (int i=0; i < number; i++) {
        //Vector3 world_camera_dir = Vector3Subtract(GetCamera()->rl_camera.target, GetCamera()->rl_camera.position);

        Vector3 mat_z = Vector3Subtract(render_camera_pos, spheres[i].center);  // In world space
        float camera_distance = Vector3Length(mat_z);
        Vector3 mat_y = { 0.0f, 1.0f, 0.0f };  // Chosen arbitrarily

        Vector3OrthoNormalize(&mat_z, &mat_y);
        Vector3 mat_x = Vector3CrossProduct(mat_y, mat_z);

        float render_radius = spheres[i].radius;
        float offset = render_radius * render_radius / camera_distance;  // Offset from midpoint to plane in render-world coordinate system
        float h = sqrt(render_radius * render_radius - offset * offset);  // plane scale in render-world coordinate system
        //float h = render_radius * sqrt(1 - 1/camera_distance);  // plane scale in render-world coordinate system

        planet_render_buffers::transform[i] = MatrixFromColumns(
            Vector3Scale(mat_x, h),
            Vector3Scale(mat_y, h),
            Vector3Scale(mat_z, h)
        );	// Model to view such that model faces you

        planet_render_buffers::transform[i].m12 = spheres[i].center.x + mat_z.x * offset;
        planet_render_buffers::transform[i].m13 = spheres[i].center.y + mat_z.y * offset;
        planet_render_buffers::transform[i].m14 = spheres[i].center.z + mat_z.z * offset;
        
        planet_render_buffers::radius[i] = render_radius;
        planet_render_buffers::centerPos[i] = spheres[i].center;
        planet_render_buffers::normal[i] = mat_z;
        ColorToFloatBuffer(&planet_render_buffers::color[i*4], spheres[i].color);

        //DebugDrawTransform(new_model_matrix);

        //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_x));
        //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_y));
        //DebugDrawLineRenderSpace(render_position, Vector3Add(render_position, mat_z));
    }

    // Global parameters
    float line_thickness = 2.0f;
    float screen_width = GetScreenWidth();
    
    float fill_color4[4];
    ColorToFloatBuffer(fill_color4, Palette::bg);

    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());

    rlEnableShader(planet_shader::shader.id);
    rlSetUniform(planet_shader::fillColor, fill_color4, SHADER_UNIFORM_VEC4, 1);
    rlSetUniform(planet_shader::space, &line_thickness, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(planet_shader::edge, &line_thickness, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(planet_shader::screenWidth, &screen_width, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(planet_shader::cameraPos, &render_camera_pos, SHADER_UNIFORM_VEC3, 1);

    rlSetUniform(planet_shader::rimColor, planet_render_buffers::color, SHADER_UNIFORM_VEC4, number);
    rlSetUniform(planet_shader::radius, planet_render_buffers::radius, SHADER_UNIFORM_FLOAT, number);
    rlSetUniform(planet_shader::centerPos, planet_render_buffers::centerPos, SHADER_UNIFORM_VEC3, number);
    rlSetUniform(planet_shader::normal, planet_render_buffers::normal, SHADER_UNIFORM_VEC3, number);

    rlSetUniformMatrix(planet_shader::mvp, mvp);
    rlSetUniformMatrixArray(planet_shader::transform, planet_render_buffers::transform, number);

    rlEnableVertexArray(planet_render_buffers::vao);
    rlDrawVertexArray(0, planet_render_buffers::VALS_PER_QUAD * number);
    rlDisableVertexArray();

    rlDisableShader();

    //for (int i=0; i < number; i++) {
    //    DrawSphereWires(spheres[i].center, spheres[i].radius, 16, 16, WHITE);
    //}
}
