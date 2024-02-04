#include "render_server.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"
#include "planet.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "debug_drawing.hpp"

void Icon3D::Draw() const {
    Vector3 render_pos = GameCamera::WorldToRender(world_pos);
    Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
    draw_pos = Vector2Add(draw_pos, offset);
    
    if (draw_pos.y < -20 || draw_pos.x < -20) {
        return;
    }

    //Vector3 cam_z = Vector3Subtract(render_pos, GetCamera()->rl_camera.position);
    Vector3 cam_z = Vector3Subtract(GetCamera()->rl_camera.target, GetCamera()->rl_camera.position);
    Vector3 cam_y = { 0.0f, 1.0f, 0.0f };
    Vector3OrthoNormalize(&cam_z, &cam_y);
    Vector3 cam_x = Vector3CrossProduct(cam_z, cam_y);

    Vector3 offset_render_pos = GetFinalRenderPos();
    float render_scale = GetFinalRenderScale();

    Rectangle source = atlas_pos.GetRect();

    DrawBillboardPro(
        GetCamera()->rl_camera, rendering::GetIconAtlas(0), source, 
        offset_render_pos, cam_y, 
        { render_scale, render_scale }, Vector2Zero(), 0.0f, 
        color
    );
}

Vector3 Icon3D::GetFinalRenderPos() const {
    Vector3 render_pos = GameCamera::WorldToRender(world_pos);
    if (Vector2LengthSqr(offset) == 0) {
        return render_pos;
    } else {
        Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
        draw_pos = Vector2Add(draw_pos, offset);

        Ray ray = GetMouseRay(draw_pos, GetCamera()->rl_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        return Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
    }
}

float Icon3D::GetFinalRenderScale() const {
    Vector3 render_pos = GameCamera::WorldToRender(world_pos);
    Vector2 draw_pos = GetCamera()->GetScreenPos(world_pos);
    draw_pos = Vector2Add(draw_pos, offset);
    if (Vector2LengthSqr(offset) == 0) {
        Ray ray = GetMouseRay(Vector2Add(draw_pos, {0.707, 0.707}), GetCamera()->rl_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        Vector3 offset_pos = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));
        return Vector3Distance(offset_pos, render_pos) * scale;  // finite difference
    } else {
        Ray ray = GetMouseRay(draw_pos, GetCamera()->rl_camera);
        float dist = Vector3Distance(ray.position, render_pos);
        Vector3 offset_render_pos = Vector3Add(ray.position, Vector3Scale(ray.direction, dist));

        Vector3 projected_diff = Vector3Subtract(offset_render_pos, render_pos);
        projected_diff = Vector3Subtract(projected_diff, Vector3Project(projected_diff, ray.direction));
        return Vector3Length(projected_diff) / Vector2Length(offset) * scale;  // finite difference
    }
}

void _DrawTrajectories() {
    // Draw planet trajectories
    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        OrbitSegment segment = OrbitSegment(&GetPlanetByIndex(i)->orbit);
        RenderOrbit(&segment, 256, OrbitRenderMode::Gradient, GetPlanetByIndex(i)->GetColor());
    }
    
    // Draw ship trajectories
    for (auto it = GetShips()->alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        ship->DrawTrajectories();
    }

    // Draw planet trajectories
    GetTransferPlanUI()->Draw3D();
}

void _UpdateShipIcons() {
    static float x_offsets[4];
    static int type_count[4];
    IDList ships_in_orbit;

    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        // Collect ships
        ships_in_orbit.Clear();
        GetShips()->GetOnPlanet(&ships_in_orbit, RID(i, EntityType::PLANET), 0xFFFFFFFFU);

        float pixel_size = GetCamera()->MeasurePixelSize(GameCamera::WorldToRender(GetPlanetByIndex(i)->position.cartesian));
        float planet_pixel_size = GameCamera::WorldToRender(GetPlanetByIndex(i)->radius) / pixel_size;

        float mouse_distance = Vector2Distance(
            GetCamera()->GetScreenPos(GetPlanetByIndex(i)->position.cartesian),
            GetMousePosition()
        );
        float mouse_closeness = Smoothstep(100 + planet_pixel_size, 50 + planet_pixel_size, mouse_distance);
        float stride = Lerp(8.0f, 20.0f, mouse_closeness);
        float icon_size = Lerp(8.0f, 18.0f, mouse_closeness);

        // clean buffers
        for(int j=0; j < 4; j++) type_count[j] = 0;
        x_offsets[0] = -planet_pixel_size - stride*2;
        x_offsets[1] = -planet_pixel_size - stride;
        x_offsets[2] =  planet_pixel_size + stride;
        x_offsets[3] =  planet_pixel_size + stride*2;
        
        for (int j=0; j < ships_in_orbit.size; j++) {
            Ship* ship = GetShip(ships_in_orbit[j]);

            int type = ship->GetShipType();
            Vector2 draw_offset = { x_offsets[type], type_count[type]++ * stride };
            ship->DrawIcon(icon_size, draw_offset);
        }
    }

    // Handle orphan ships
    ships_in_orbit.Clear();
    GetShips()->GetOnPlanet(&ships_in_orbit, GetInvalidId(), 0xFFFFFFFFU);
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
        ship->DrawIcon(icon_size, Vector2Zero());
    }
}

void RenderServer::Draw() {
    BeginMode3D(GetCamera()->rl_camera);
    RenderSkyBox();
 
    // Draw Saturn
    RenderPerfectSphere(DVector3::Zero(), GetPlanets()->GetParentNature()->radius, Palette::ui_main);

    // Rings are hardcoded, because why not?
    RenderRings(DVector3::Up(), 74.0e+6, 92.0e+6, Palette::ui_main);
    RenderRings(DVector3::Up(), 94.0e+6, 117.58e+6, Palette::ui_main);
    RenderRings(DVector3::Up(), 122.17e+6, 136.775e+6, Palette::ui_main);

    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++) {    
        // planet spheres
        RenderPerfectSphere(
            GetPlanetByIndex(i)->position.cartesian, 
            GetPlanetByIndex(i)->radius, 
            GetPlanetByIndex(i)->GetColor()
        );
    }

    _DrawTrajectories();

    _UpdateShipIcons();

    // Draw icons
    BeginShaderMode(rendering::GetIconShader());
    for (auto it = icons.GetIter(); it; it++) {
        icons[it.GetId()]->Draw();
    }
    EndShaderMode();

    DebugFlush3D();
    EndMode3D();
}

RID RenderServer::AllocateIcon3D(float scale, Vector2 offset, AtlasPos atlas_pos, 
                                 Color color, DVector3 world_pos) {
    Icon3D* icon;
    RID id = icons.Allocate(&icon);
    icon->scale = scale;
    icon->offset = offset;
    icon->atlas_pos = atlas_pos;
    icon->color = color;
    icon->world_pos = world_pos;
    return id;
}

void RenderServer::UpdateIcon3D(RID id, float scale, Vector2 offset, AtlasPos atlas_pos, 
                                Color color, DVector3 world_pos) {
    Icon3D* icon = icons.Get(id);
    icon->scale = scale;
    icon->offset = offset;
    icon->atlas_pos = atlas_pos;
    icon->color = color;
    icon->world_pos = world_pos;
}