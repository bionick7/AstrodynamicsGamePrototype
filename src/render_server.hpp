#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H

#include "basic.hpp"
#include "id_allocator.hpp"
#include "dvector3.hpp"
#include "render_utils.hpp"
#include "ui.hpp"

struct EmbeddedScene {
    int mesh_count;
    Matrix* transforms = NULL;
    WireframeMesh* meshes = NULL;
    RenderTexture render_target;

    Camera camera;
    int render_width = 0, render_height = 0;

    EmbeddedScene() = default;
    ~EmbeddedScene();

    void Make(int p_mesh_count, int p_render_width, int p_render_height);
    void UpdateTurntableCamera(float yaw_rate, float pitch);
    void Render();
};

struct Icon3D {
    float scale;
    Vector2 offset;
    AtlasPos atlas_pos;

    Color color;
    DVector3 world_pos;

    void Draw() const;
    Vector3 GetFinalRenderPos() const;
    float GetFinalRenderScale() const;
};

struct Text3D {
    float scale;
    Vector2 offset;
    const char* text;

    Color color;
    DVector3 world_pos;
    text_alignment::T alignment;

    void Draw() const;
};

RenderTexture2D LoadRenderTextureWithDepth(int width, int height);
void UnloadRenderTextureWithDepth(RenderTexture2D target);

struct RenderServer {
    IDAllocatorList<Icon3D, EntityType::ICON3D> icons;
    IDAllocatorList<Text3D, EntityType::TEXT3D> text_labels_3d;
    IDAllocatorList<EmbeddedScene, EntityType::EMBEDDED_SCENE> embedded_scenes;
    RenderTexture2D render_targets[2];
    
    double animation_time = 0;  // Can drive animations
    
    void OnScreenResize();
    void Draw();
};

#endif  // RENDER_SERVER_H