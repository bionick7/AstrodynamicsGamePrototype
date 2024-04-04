#ifndef WIREFRAME_MESH_H
#define WIREFRAME_MESH_H

#include "basic.hpp"

struct WireframeMesh {
    int vertex_count;
    int triangle_count;
    int line_count;

    BoundingBox bounding_box;

    int vao_triangles = -1;
    int vao_lines = -1;
    int vbo_triangles = -1;
    int vbo_lines = -1;
};

WireframeMesh LoadWireframeMesh(const char* filepath);
WireframeMesh LoadTestWireframeMesh();
void UnLoadWireframeMesh(WireframeMesh wireframe_mesh);
bool IsWireframeReady(WireframeMesh wireframe_mesh);

#endif  // WIREFRAME_MESH_H