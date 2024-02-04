#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H

#include "basic.hpp"
#include "id_allocator.hpp"
#include "dvector3.hpp"
#include "render_utils.hpp"

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

struct RenderServer {
    IDAllocatorList<Icon3D, EntityType::ICON3D> icons;

    void Draw();
    RID AllocateIcon3D(float scale, Vector2 offset, AtlasPos atlas_pos, 
                       Color color, DVector3 world_pos);
    void UpdateIcon3D(RID id, float scale, Vector2 offset, AtlasPos atlas_pos, 
                      Color color, DVector3 world_pos);
};

#endif  // RENDER_SERVER_H