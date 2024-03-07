#ifndef WIREFRAME_MESH_H
#define WIREFRAME_MESH_H

#include "basic.hpp"

//#define WIREFRAME_USE_NATIVE_BUFFERS  // Better, but harder (unsolved)

struct WireframeMesh {
    int vertex_count;
    int line_count;

    float* vertecies;
    float* vertex_distances;
    int* lines;

    BoundingBox bounding_box;

#ifdef WIREFRAME_USE_NATIVE_BUFFERS
    unsigned int vao;
    unsigned int vbo_vertecies;
    unsigned int vbo_lines;
#endif
};

WireframeMesh LoadWireframeMesh(const char* filepath);
void UnLoadWireframeMesh(WireframeMesh wireframe_mesh);

#endif  // WIREFRAME_MESH_H