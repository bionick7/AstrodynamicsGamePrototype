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

    void Load() {
        LOAD_SHADER_FS(sdf_shader)
        LOAD_SHADER_UNIFORM(sdf_shader, depth)
    }

    void UnLoad() {
        UnloadShader(shader);
    }
}

namespace ui_shader {
    Shader shader;
    int depth = -1;

    void Load() {
        LOAD_SHADER_FS(ui_shader)
        LOAD_SHADER_UNIFORM(ui_shader, depth)
    }

    void UnLoad() {
        UnloadShader(shader);
    }
}

int GetCharacterIndex(Vector2 position, Font font, const char *text, float fontSize, float spacing) {
    // Returns byte offset into the char array, rather than glyph position
    // Returns -1 if position is outside text region
    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            textOffsetY += 20;
            textOffsetX = 0.0f;
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };
            if (CheckCollisionPointRec(position, char_rect)) {
                return i;
            }

            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return -1;
}

Rectangle GetTextRect(int token_start, int token_end, int token_from, Font font, const char *text, float fontSize, float spacing) {
    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    Rectangle rect = Rectangle{ 0, 0, 0, 20 };

    for (int i = 0; i < size;) {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);
        if (i == token_start) {
            rect.x = textOffsetX;
            rect.y = textOffsetY;
        }
        else if (i == token_end) {
            rect.width = textOffsetX - rect.x;
            return rect;
        }
        if (codepoint == '\n') {
            if (i > token_from) {
                rect.width = textOffsetX - rect.x;
                return rect;
            }
            textOffsetY += 20;
            textOffsetX = 0.0f;
            if (i > token_start) {
                rect.x = textOffsetX;
                rect.y = textOffsetY;
            }
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            //Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };


            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return rect;
}

std::vector<Rectangle> GetTextRects(const TokenList *tokens, Font font, const char *text, float fontSize, float spacing) {
    std::vector<Rectangle> res = std::vector<Rectangle>();
    if (tokens->length == 0) return res;

    if (font.texture.id == 0) font = GetFontDefault();
    int size = TextLength(text);

    int textOffsetY = 0;
    float textOffsetX = 0.0f;

    float scaleFactor = fontSize/font.baseSize;

    int current_token_index = 0;
    bool in_token = false;

    Rectangle rect = Rectangle{ 0, 0, 0, 20 };

    int current_start_pos;
    int current_end_pos;

    for (int i = 0; i < size;) {
        if (current_token_index < tokens->length) {
            current_start_pos = tokens->start_positions[current_token_index];
            current_end_pos = tokens->end_positions[current_token_index];
        } else {
            current_start_pos = -1;
            current_end_pos = -1;
        }

        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);
        if (i == current_start_pos) {
            rect.x = textOffsetX;
            rect.y = textOffsetY;
            in_token = true;
        }
        else if (i == current_end_pos) {
            rect.width = textOffsetX - rect.x;
            res.push_back(rect);
            in_token = false;
            current_token_index++;
        }
        if (codepoint == '\n') {
            if (in_token) {
                res.push_back(rect);
            }
            textOffsetY += 20;
            textOffsetX = 0.0f;
            if (in_token) {
                rect.x = textOffsetX;
                rect.y = textOffsetY;
            }
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            //Rectangle char_rect = { textOffsetX, textOffsetY, x_increment, fontSize };


            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }
    return res;
}

void InternalDrawText(const char *text, Vector2 position, Color color) {
    InternalDrawTextEx(GetCustomDefaultFont(), text, position, DEFAULT_FONT_SIZE, 1, color, GetScreenRect(), 0);
}

void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    InternalDrawTextEx(font, text, position, fontSize, spacing, tint, GetScreenRect(), 0);
}

void InternalDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, 
                        Color tint, Rectangle render_rect, uint8_t z_layer) {
    if (font.texture.id == 0) font = GetFontDefault();  // (Raylib cmt) Security check in case of not valid font

    int size = TextLength(text);    // (Raylib cmt) Total size in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // (Raylib cmt) Offset between lines (on linebreak '\n')
    float textOffsetX = 0.0f;       // (Raylib cmt) Offset X to next character to draw

    float scaleFactor = fontSize/font.baseSize;         // (Raylib cmt) Character quad scaling factor

    if (GetSettingBool("sdf_text", false)) {
        RELOAD_IF_NECAISSARY(sdf_shader)
        float z_layer_f = 1.0f - z_layer / 256.0f;
        SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(sdf_shader::shader);
    }

    for (int i = 0; i < size;) {
        // (Raylib cmt)Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == '\n') {
            // (Raylib cmt) NOTE: Line spacing is a global variable, use SetTextLineSpacing() to setup
            textOffsetY += 20;  // TODO, raylib doesn't expose line space, so how to?
            textOffsetX = 0.0f;
        } else {
            float x_increment = (float)font.recs[index].width*scaleFactor + spacing;
            Vector2 char_pos = { position.x + textOffsetX, position.y + textOffsetY };

            // This part is custom

            Vector2 char_pos2 = { position.x + textOffsetX + x_increment, position.y + textOffsetY + fontSize };
            if (
                (codepoint != ' ') && (codepoint != '\t')
                && CheckCollisionPointRec(char_pos, render_rect)
                && CheckCollisionPointRec(char_pos2, render_rect)
            ) {
                DrawTextCodepoint(font, codepoint, char_pos, fontSize, tint);
            }

            if (font.glyphs[index].advanceX == 0) textOffsetX += x_increment;
            else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // (Raylib cmt) Move text bytes counter to next codepoint
    }

    if (GetSettingBool("sdf_text", false))
        EndShaderMode();
}

void DrawTextureSDF(Texture2D texture, Rectangle source, Rectangle dest, 
                    Vector2 origin, float rotation, Color tint, uint8_t z_layer) {
    RELOAD_IF_NECAISSARY(sdf_shader)
    BeginShaderMode(sdf_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    SetShaderValue(sdf_shader::shader, sdf_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
    DrawTexturePro(texture, source, dest, origin, rotation, tint);
    EndShaderMode();
}

void BeginRenderInUIMode(uint8_t z_layer) {
    RELOAD_IF_NECAISSARY(ui_shader)
    BeginShaderMode(ui_shader::shader);
    float z_layer_f = 1.0f - z_layer / 256.0f;
    SetShaderValue(ui_shader::shader, ui_shader::depth, &z_layer_f, SHADER_UNIFORM_FLOAT);
}

void EndRenderInUIMode() {
    EndShaderMode();
}

namespace orbit_shader {
    Shader shader;
    int semi_latus_rectum = -1;
    int eccentricity = -1;
    int orbit_transform = -1;
    int current_anomaly = -1;

    int color = -1;
    int render_mode = -1;

    void Load() {
        LOAD_SHADER(orbit_shader)
        LOAD_SHADER_UNIFORM(orbit_shader, semi_latus_rectum)
        LOAD_SHADER_UNIFORM(orbit_shader, eccentricity)
        LOAD_SHADER_UNIFORM(orbit_shader, orbit_transform)
        LOAD_SHADER_UNIFORM(orbit_shader, current_anomaly)
        LOAD_SHADER_UNIFORM(orbit_shader, render_mode)
        LOAD_SHADER_UNIFORM(orbit_shader, color)
    }

    void UnLoad() {
        UnloadShader(shader);
    }
}

void RenderOrbit(const OrbitSegment *segment, int point_count, OrbitRenderMode::T render_mode, Color color) {
    RELOAD_IF_NECAISSARY(orbit_shader)

    const Orbit* orbit = segment->orbit;

    float p = (float) (orbit->sma * (1 - orbit->ecc*orbit->ecc) / GameCamera::space_scale);
    float ecc = (float) orbit->ecc;
    Vector3 mat_x = (Vector3) orbit->periapsis_dir;
    Vector3 mat_y = (Vector3) orbit->normal;
    Vector3 mat_z = Vector3CrossProduct(mat_y, mat_x);

    float current_focal_anomaly = orbit->GetPosition(GlobalGetNow()).θ;

    SetShaderValue(orbit_shader::shader, orbit_shader::semi_latus_rectum, &p, SHADER_UNIFORM_FLOAT);
    SetShaderValue(orbit_shader::shader, orbit_shader::eccentricity, &ecc, SHADER_UNIFORM_FLOAT);
    SetShaderValue(orbit_shader::shader, orbit_shader::current_anomaly, &current_focal_anomaly, SHADER_UNIFORM_FLOAT);
    int render_mode_int = render_mode;
    SetShaderValue(orbit_shader::shader, orbit_shader::render_mode, &render_mode_int, SHADER_UNIFORM_INT);
    SetShaderValueMatrix(orbit_shader::shader, orbit_shader::orbit_transform, MatrixFromColumns(mat_x, mat_y, mat_z));

    float color4[4];
    _ColorToFloat4Buffer(color4, color);
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
        float focal_bound1 = segment->bound1.θ;
        float focal_bound2 = segment->bound2.θ;
        if (focal_bound2 < focal_bound1) focal_bound2 += PI*2;
        float delta = (focal_bound2 - focal_bound1) / (float) point_count;
        for (int j=0; j < point_count; j++) {
            rlVertex3f(focal_bound1 + j * delta, 0, 0);
            rlVertex3f(focal_bound1 + (j + 1) * delta, 0, 0);
        }
    }

    rlEnd();
    EndShaderMode();
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

    void UnLoad() {
        UnloadShader(shader);
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

    void UnLoad() {
        UnloadShader(shader);
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

    Texture2D starmap;
    float palette_vec[4*4];
    
    void Load() {
        LOAD_SHADER(skybox_shader)
        LOAD_SHADER_UNIFORM(skybox_shader, resolution)
        LOAD_SHADER_UNIFORM(skybox_shader, fov)
        LOAD_SHADER_UNIFORM(skybox_shader, palette)
        LOAD_SHADER_UNIFORM(skybox_shader, matCamera)
        LOAD_SHADER_UNIFORM(skybox_shader, starMap)

        skybox_shader::starmap = LoadTexture("resources/textures/starmap_4k.jpg");
        //skybox_shader::starmap = LoadTexture("resources/textures/vsauce.png");
        
        _ColorToFloat4Buffer(&palette_vec[4*0], Palette::bg);
        _ColorToFloat4Buffer(&palette_vec[4*1], Palette::ui_dark);
        _ColorToFloat4Buffer(&palette_vec[4*3], Palette::ui_alt);
        _ColorToFloat4Buffer(&palette_vec[4*2], Palette::ui_alt);
    }

    void UnLoad() {
        UnloadTexture(skybox_shader::starmap);
        UnloadShader(shader);
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
    SetShaderValueTexture(skybox_shader::shader, skybox_shader::starMap, skybox_shader::starmap);

    //skybox_shader::starmap = LoadTexture("resources/textures/vsauce.png");
    DrawTexture(skybox_shader::starmap, INT32_MAX, INT32_MAX, WHITE);
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
        shader = LoadShader(NULL, "resources/shaders/postprocessing_shader.fs");
        LOAD_SHADER_UNIFORM(postprocessing_shader, depthMap);
    }

    void UnLoad() {
        UnloadShader(shader);
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

void ReloadShaders() {
    sdf_shader::UnLoad();
    orbit_shader::UnLoad();
    planet_shader::UnLoad();
    rings_shader::UnLoad();
    skybox_shader::UnLoad();
    postprocessing_shader::UnLoad();
    
    sdf_shader::Load();
    orbit_shader::Load();
    planet_shader::Load();
    rings_shader::Load();
    skybox_shader::Load();
    postprocessing_shader::Load();
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
