#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H

#include "basic.hpp"
#include "id_allocator.hpp"
#include "dvector3.hpp"
#include "render_utils.hpp"
#include "ui.hpp"

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
    TextAlignment::T alignment;

    void Draw() const;
};

struct RenderServer {
    IDAllocatorList<Icon3D, EntityType::ICON3D> icons;
    IDAllocatorList<Text3D, EntityType::TEXT3D> text_labels_3d;
    RenderTexture2D render_targets[2];
    
    void OnScreenResize();

    void Draw();
};

#endif  // RENDER_SERVER_H