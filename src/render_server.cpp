#include "render_server.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "planet.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"

#include "rlgl.h"

EmbeddedScene::~EmbeddedScene() {
    delete[] meshes;
    delete[] transforms;
}

void EmbeddedScene::Make(int p_mesh_count, int p_render_width, int p_render_height) {
    mesh_count = p_mesh_count;
    meshes = new WireframeMesh[mesh_count];
    transforms = new Matrix[mesh_count];
    render_width = p_render_width;
    render_height = p_render_width;

    camera.position = {0, 0, 5};
    camera.target = Vector3Zero();
    camera.up = {0, 1, 0};
    camera.fovy = 45;
    camera.projection = CAMERA_PERSPECTIVE;
}

void EmbeddedScene::UpdateTurntableCamera(float yaw_rate, float pitch) {
    float yaw = GetTime() * yaw_rate;
    const float distance = 5;
    camera.position = {
        cosf(yaw) * cosf(pitch) * distance,
        -sinf(pitch) * distance,
        sinf(yaw) * cosf(pitch) * distance,
    };
    camera.target = Vector3Zero();
    camera.up = { 0, 1, 0 };
    camera.fovy = 45;
    camera.projection = CAMERA_PERSPECTIVE;
}

void EmbeddedScene::Render() {
    BeginTextureMode(render_target);
    BeginMode3D(camera);
    ClearBackground(ColorAlpha(Palette::bg, 0));

    // Set up render environment
    if (render_target.texture.width != render_width || 
        render_target.texture.height != render_height
    ) {
        render_target.texture.width = render_width;
        render_target.texture.height = render_height;
        if (IsRenderTextureReady(render_target))
            UnloadRenderTextureWithDepth(render_target);
        render_target = LoadRenderTextureWithDepth(render_width, render_height);
    }

    for (int i=0; i < mesh_count; i++) {
        if (IsWireframeReady(meshes[i])) {
            RenderWireframeMesh(meshes[i], transforms[i], Palette::bg, Palette::ui_main);
        }
    }

    EndMode3D();
    EndTextureMode();
}

namespace icon_shader {
    Shader shader;

    void Load() { LOAD_SHADER(icon_shader) }
}

void Icon3D::Draw() const {
    Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
    draw_pos = Vector2Add(draw_pos, offset);
    
    if (draw_pos.y < -20 || draw_pos.x < -20) {  // TODO: Un-hardcode
        return;
    }

    Vector3 cam_z = Vector3Subtract(GetCamera()->macro_camera.target, GetCamera()->macro_camera.position);
    Vector3 cam_y = { 0.0f, 1.0f, 0.0f };
    Vector3OrthoNormalize(&cam_z, &cam_y);

    Vector3 offset_render_pos = GetFinalRenderPos();
    float render_scale = GetFinalRenderScale();

    Rectangle source = atlas_pos.GetRect(ATLAS_SIZE);

    DrawBillboardPro(
        GetCamera()->macro_camera, GetUI()->GetIconAtlasSDF(), source, 
        offset_render_pos, cam_y, 
        { render_scale, render_scale }, Vector2Zero(), 0.0f, 
        color
    );
}

Vector3 Icon3D::GetFinalRenderPos() const {
    Vector3 render_pos = GameCamera::WorldToMacro(world_pos);
    if (Vector2LengthSqr(offset) == 0) {
        return render_pos;
    } else {
        Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
        draw_pos = Vector2Add(draw_pos, offset);

        Ray ray = GetMouseRay(draw_pos, GetCamera()->macro_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        return Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
    }
}

float Icon3D::GetFinalRenderScale() const {
    Vector3 render_pos = GameCamera::WorldToMacro(world_pos);
    Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
    draw_pos = Vector2Add(draw_pos, offset);
    if (Vector2LengthSqr(offset) == 0) {
        Ray ray = GetMouseRay(Vector2Add(draw_pos, {0.707, 0.707}), GetCamera()->macro_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        Vector3 offset_pos = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
        return Vector3Distance(offset_pos, render_pos) * scale;  // finite difference
    } else {
        Ray ray = GetMouseRay(draw_pos, GetCamera()->macro_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        Vector3 offset_render_pos = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));

        Vector3 projected_diff = Vector3Subtract(offset_render_pos, render_pos);
        projected_diff = Vector3Subtract(projected_diff, Vector3Project(projected_diff, ray.direction));
        return Vector3Length(projected_diff) / Vector2Length(offset) * scale;  // finite difference
    }
}

namespace text3d_shader {
    Shader shader;
    int ndcDepth = -1;
    int useSdf = -1;
    
    void Load() {
        LOAD_SHADER(text3d_shader)
        LOAD_SHADER_UNIFORM(text3d_shader, ndcDepth)
        LOAD_SHADER_UNIFORM(text3d_shader, useSdf)
    }
}

void Text3D::Draw() const {
    if (!GetCamera()->IsInView(world_pos)) {
        return;
    }

    Vector3 render_pos = GameCamera::WorldToMacro(world_pos);

    Vector4 render_pos4 = { render_pos.x, render_pos.y, render_pos.z, 1.0f };
    Vector4 view_pos = QuaternionTransform(render_pos4, GetCamera()->ViewMatrix());
    Vector4 clip_pos = QuaternionTransform(view_pos, GetCamera()->ProjectionMatrix());
    Vector3 ndcPos = { clip_pos.x/clip_pos.w, -clip_pos.y/clip_pos.w, clip_pos.z/clip_pos.w };
    if (ndcPos.x < -1 || ndcPos.x > 1 || ndcPos.y < -1 || ndcPos.y > 1) {
        return;
    }

    int text_h = scale;
    Vector2 screen_pos;
    screen_pos.x = (ndcPos.x + 1.0f)/2.0f * GetScreenWidth() + offset.x;
    screen_pos.y = (ndcPos.y + 1.0f)/2.0f * GetScreenHeight() + offset.y;
    Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(text_h), text, text_h, 1);
    screen_pos = ApplyAlignment(screen_pos, text_size, alignment);
    
    if (GetSettingBool("draw_textrects", false)) {
        DrawRectangleLines(screen_pos.x, screen_pos.y, text_size.x, text_size.y, RED);
    }

    SetShaderValue(text3d_shader::shader, text3d_shader::ndcDepth, &ndcPos.z, SHADER_UNIFORM_FLOAT);
    int use_sdf = GetSettingBool("sdf_text", false) ? 1 : 0;
    SetShaderValue(text3d_shader::shader, text3d_shader::useSdf, &use_sdf, SHADER_UNIFORM_INT);

    // Cannot render in a single batch, because of the ndcPos.z uniform
    BeginShaderMode(text3d_shader::shader);
    DrawTextEx(GetCustomDefaultFont(text_h), text, screen_pos, text_h, 1, color);
    EndShaderMode();
}

void _DrawTrajectories() {
    // Draw planet trajectories
    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        GetPlanetByIndex(i)->Draw3D();
    }
    
    // Draw ship trajectories
    for (auto it = GetShips()->alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        ship->DrawTrajectories();
    }

    // Draw transfer plan trajectories
    GetTransferPlanUI()->Draw3D();
}

void _UpdateShipIcons() {
    static int x_offsets[4];
    static int y_offsets[4];

    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        Planet* planet = GetPlanetByIndex(i);

        // Update planet text3d

        Text3D* text3d = GetRenderServer()->text_labels_3d.GetOrAllocate(&planet->text3d);

        float radius_px = GetCamera()->MeasurePixelSize(GameCamera::WorldToMacro(text3d->world_pos));
        text3d->scale = DEFAULT_FONT_SIZE;
        text3d->offset = { 0, radius_px + 3 };
        text3d->text = planet->name.GetChar();
        text3d->color = Palette::ui_main;
        text3d->world_pos = planet->position.cartesian;
        text3d->alignment = text_alignment::HCENTER | text_alignment::BOTTOM;

        // Collect ships

        float pixel_size = GetCamera()->MeasurePixelSize(GameCamera::WorldToMacro(GetPlanetByIndex(i)->position.cartesian));
        float planet_pixel_size = GameCamera::WorldToMacro(GetPlanetByIndex(i)->radius) / pixel_size;

        float mouse_distance = Vector2Distance(
            GetCamera()->GetScreenPos(GetPlanetByIndex(i)->position.cartesian),
            GetMousePosition()
        );
        float grow_factor = Smoothstep(100 + planet_pixel_size, 50 + planet_pixel_size, mouse_distance);
        float stride = Lerp(8.0f, 20.0f, grow_factor);

        // clean buffers
        for(int j=0; j < 4; j++) y_offsets[j] = stride / 2;
        x_offsets[0] = -planet_pixel_size - stride;
        x_offsets[1] = -planet_pixel_size - stride;
        x_offsets[2] =  planet_pixel_size + stride;
        x_offsets[3] =  planet_pixel_size + stride;
        
        for (int j=0; j < planet->cached_ship_list.size; j++) {
            Ship* ship = GetShip(planet->cached_ship_list[j]);
            ship->DrawIcon(x_offsets, y_offsets, grow_factor);
        }
    }

    // Handle orphan ships
    IDList ships_in_orbit;
    GetShips()->GetOnPlanet(&ships_in_orbit, GetInvalidId(), ship_selection_flags::ALL);
    for (int j=0; j < ships_in_orbit.size; j++) {
        Ship* ship = GetShip(ships_in_orbit[j]);
        // TODO: offsets to draw fleets

        float mouse_distance = Vector2Distance(
            GetCamera()->GetScreenPos(ship->position.cartesian),
            GetMousePosition()
        );
        float mouse_closeness = Smoothstep(100, 50, mouse_distance);
        float stride = Lerp(10.0f, 25.0f, mouse_closeness);
        float icon_size = Lerp(10.0f, 18.0f, mouse_closeness);
        for(int j=0; j < 4; j++) {
            x_offsets[j] = 0;
            y_offsets[j] = 0;
        }
        ship->DrawIcon(x_offsets, y_offsets, mouse_closeness);
    }
}

// Directly from raylib depth buffer example
// Load custom render texture, create a writable depth texture buffer
RenderTexture2D LoadRenderTextureWithDepth(int width, int height) {
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, 
                            RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, 
                            RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) 
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
        SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload render texture from GPU memory (VRAM)
void UnloadRenderTextureWithDepth(RenderTexture2D target) {
    if (target.id > 0) {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}

void RenderServer::QueueConicDraw(ConicRenderInfo conic_render_info) {
    conic_queue.Append(conic_render_info);
}

void RenderServer::QueueSphereDraw(SphereRenderInfo sphere_render_info) {
    if (sphere_render_info.radius == 0) {
        // Don't draw degenerate spheres (situational optimization)
        return;
    }
    sphere_queue.Append(sphere_render_info);
}

void RenderServer::OnScreenResize() {
    for (int i=0; i < render_layers::MAX; i++) {
        if (IsRenderTextureReady(render_targets[i])) {
            UnloadRenderTextureWithDepth(render_targets[i]);
        }
        render_targets[i] = LoadRenderTextureWithDepth(GetScreenWidth(), GetScreenHeight());
    }
}

void _BeginCameraDraw(Camera3D camera, float nearclip, float farclip) {
    // Copied Raylib func to include nearclip and farclip values customizable at runtime

    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();

    float aspect = GetScreenWidth() / (float) GetScreenHeight();

    // NOTE: zNear and zFar values are important when computing depth buffer values
    if (camera.projection == CAMERA_PERSPECTIVE) {
        // Setup perspective projection
        double top = nearclip*tan(camera.fovy*0.5*DEG2RAD);
        double right = top*aspect;

        rlFrustum(-right, right, -top, top, nearclip, farclip);
    }

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    // Setup Camera view
    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
    rlMultMatrixf(MatrixToFloat(matView));      // Multiply modelview matrix by view matrix (camera)

    rlEnableDepthTest();
}

void RenderServer::Draw() {
    animation_time += GetFrameTime();
    if (render_targets[0].texture.width != GetScreenWidth() || render_targets[0].texture.height != GetScreenHeight()) {
        OnScreenResize();
    }

    for (int i=0; i < render_layers::MAX; i++) {
        if (!IsRenderTextureReady(render_targets[i])) {
            render_targets[i] = LoadRenderTextureWithDepth(GetScreenWidth(), GetScreenHeight());
        }
    }

    //WireframeMesh test_mesh = assets::GetWireframe("resources/meshes/test/ship_contours.obj");
    //WireframeMesh test_mesh = assets::GetWireframe("resources/meshes/ships/shp_light_transport.obj");
    PushTimer();
    for (auto it = embedded_scenes.GetIter(); it; it++) {
        EmbeddedScene* scene = embedded_scenes.Get(it.GetId());
        scene->Render();
    }
    PopAndReadTimer("Embedded Scenes", true);

    PushTimer();
    BeginTextureMode(render_targets[render_layers::PLANETS]);
        ClearBackground(WHITE);  // Very important apparently

        //RenderWireframeMesh(test_mesh, MatrixIdentity(), Palette::bg, Palette::ui_main);

        // True 3D - Planets
        _BeginCameraDraw(GetCamera()->macro_camera, GetCamera()->MacroNearClipPlane(), GetCamera()->MacroFarClipPlane());
            RenderSkyBox();

            // Saturn and rings are hardcoded for now
            QueueSphereDraw(SphereRenderInfo::FromWorldPos(DVector3::Zero(), GetPlanets()->GetParentNature()->radius, Palette::ui_main));
            RenderRings(DVector3::Up(), 74.0e+6, 92.0e+6, Palette::ui_main);
            RenderRings(DVector3::Up(), 94.0e+6, 117.58e+6, Palette::ui_main);
            RenderRings(DVector3::Up(), 122.17e+6, 136.775e+6, Palette::ui_main);

            _DrawTrajectories();
            _UpdateShipIcons();
            // Draw icons
            RELOAD_IF_NECESSARY(icon_shader)
            BeginShaderMode(icon_shader::shader);
            for (auto it = icons.GetIter(); it; it++) {
                icons[it.GetId()]->Draw();
            }

            // Draw queued objects (split it into sets of PRIMITIVE_BATCH_SIZE)
            for (int i=0; i < sphere_queue.Count(); i += PRIMITIVE_BATCH_SIZE) {
                int count = MinInt(sphere_queue.Count() - i, PRIMITIVE_BATCH_SIZE);
                RenderPerfectSpheres(&sphere_queue.buffer[i], count);
            }
            for (int i=0; i < conic_queue.Count(); i += PRIMITIVE_BATCH_SIZE) {
                int count = MinInt(conic_queue.Count() - i, PRIMITIVE_BATCH_SIZE);
                RenderOrbits(&conic_queue.buffer[i], count);
            }
            sphere_queue.Clear();
            conic_queue.Clear();

            EndShaderMode();
            DebugFlush3D();
        EndMode3D();

        // Pseudo-3d
        rlEnableDepthTest();    
            RELOAD_IF_NECESSARY(text3d_shader)
            for (auto it = text_labels_3d.GetIter(); it; it++) {
                text_labels_3d[it.GetId()]->Draw();
            }
        rlDisableDepthTest();
        GetTransferPlanUI()->Draw3DGizmos();
    
    EndTextureMode();

    // Ship scale
    BeginTextureMode(render_targets[render_layers::SHIPS]);
        ClearBackground(ColorAlpha(Palette::bg, 0));

        _BeginCameraDraw(GetCamera()->micro_camera, GetCamera()->MicroNearClipPlane(), GetCamera()->MicroFarClipPlane());
        RenderWireframeMesh(assets::GetWireframe("resources/meshes/test/ship_contours.obj"), MatrixIdentity(), Palette::bg, Palette::ui_main);
        EndMode3D();

    EndTextureMode();
    PopAndReadTimer("3D rendering", true);

    // All UI shenanigans
    PushTimer();
    BeginTextureMode(render_targets[render_layers::UI]);
        ClearBackground(ColorAlpha(Palette::bg, 0));

        rlEnableDepthTest(); 
        GetGlobalState()->DrawUI();
        rlDisableDepthTest();
    EndTextureMode();
    PopAndReadTimer("UI rendering", true);

    // Postprocessing etc.
    ClearBackground(Palette::bg);  // There are transparent areas when skipping postprocessing
    for (int i=0; i < render_layers::MAX; i++) {
        RenderDeferred(render_targets[i]);
    }
}
 