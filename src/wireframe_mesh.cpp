#include "wireframe_mesh.hpp"
#include "rlgl.h"
#include "logging.hpp"

static bool _GetNextLine(const char *origin, int *cursor) {
    int count = 0;
    bool reached_end = false;
    for (; origin[*cursor + count] != '\n'; count++) {
        if (origin[*cursor + count] == '\0') {
            reached_end = true;
            break;
        }
    }
    *cursor += count + 1;
    return !reached_end;
}

WireframeMesh LoadWireframeMesh(const char* filepath) {
    WireframeMesh mesh = {0};

    char* text = LoadFileText(filepath);
    if (text == NULL) {
        return mesh;
    }
    int text_size = strlen(text);
    int text_cursor = 0;

    int read_bytes = 0;

    int counter = 0;
    int line = 0;

    mesh.vertex_count = 0;
    mesh.line_count = 0;

    do {
        line++;
        char c = text[text_cursor];
        while (c <= 32 && c != '\n' && c != '\0') {
            c = text[++text_cursor];
        }
        //INFO("%d => '%c'(%u)", line, c,c)
        if (c == '\n' || c == '\0') continue;
        
        if (c == '#') continue;
        else if (c == 'v') mesh.vertex_count++;
        else if (c == 'l') mesh.line_count++;
        else ERROR("Unexpected character ('%c'%u) when reading at %s:%d:0", c, c, filepath, line)
    } while (_GetNextLine(text, &text_cursor));


    mesh.vertecies = new float[mesh.vertex_count*3];
    mesh.vertex_distances = new float[mesh.vertex_count];
    mesh.lines = new int[mesh.line_count*2];

    int vertex_cursor = 0;
    int line_cursor = 0;

    mesh.bounding_box.min = { INFINITY, INFINITY, INFINITY };
    mesh.bounding_box.max = {-INFINITY,-INFINITY,-INFINITY };

    text_cursor = 0;
    line = 0;
    do {
        line++;
        if (text[text_cursor] == 'v') {
            float x, y, z;
            sscanf(&text[text_cursor], "v %f %f %f", &x, &y, &z);
            mesh.vertecies[3*vertex_cursor] = x;
            mesh.vertecies[3*vertex_cursor+1] = y;
            mesh.vertecies[3*vertex_cursor+2] = z;

            if (x > mesh.bounding_box.max.x) mesh.bounding_box.max.x = x;
            if (x < mesh.bounding_box.min.x) mesh.bounding_box.min.x = x;
            if (y > mesh.bounding_box.max.y) mesh.bounding_box.max.y = y;
            if (y < mesh.bounding_box.min.y) mesh.bounding_box.min.y = y;
            if (z > mesh.bounding_box.max.z) mesh.bounding_box.max.z = z;
            if (z < mesh.bounding_box.min.z) mesh.bounding_box.min.z = z;

            if (vertex_cursor == 0) {
                mesh.vertex_distances[vertex_cursor] = 0;
            } else {
                float dx = x - mesh.vertecies[3*vertex_cursor - 3];
                float dy = y - mesh.vertecies[3*vertex_cursor - 2];
                float dz = z - mesh.vertecies[3*vertex_cursor - 1];
                float distance_to_previous = sqrtf(dx*dx + dy*dy + dz*dz);
                mesh.vertex_distances[vertex_cursor] = mesh.vertex_distances[vertex_cursor-1] + distance_to_previous;
            }
            //INFO("%d - %f", vertex_cursor, mesh.vertex_distances[vertex_cursor])

            vertex_cursor++;
        }
        else if (text[text_cursor] == 'l') {
            sscanf(&text[text_cursor], "l %d %d", &mesh.lines[line_cursor], &mesh.lines[line_cursor+1]);
            line_cursor += 2;
        }
    } while (_GetNextLine(text, &text_cursor));

    UnloadFileText(text);

    float total_distance = mesh.vertex_distances[mesh.vertex_count-1];
    for(int i=0; i < mesh.vertex_count; i++) {
        mesh.vertex_distances[i] = mesh.vertex_distances[i] / total_distance;
    }

    for(int i=0; i < mesh.line_count*2; i++) {
        mesh.lines[i]--;
    }

#ifdef WIREFRAME_USE_NATIVE_BUFFERS
    mesh.vao = rlLoadVertexArray();
    rlEnableVertexArray(mesh.vao);

    mesh.vbo_vertecies = rlLoadVertexBuffer(mesh.vertecies, mesh.vertex_count*3*sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(0);
    mesh.vbo_lines = rlLoadVertexBufferElement(mesh.lines, mesh.line_count*2*sizeof(int), false);

    rlDisableVertexArray();
    //rlDisableVertexBuffer();
    //rlDisableVertexBufferElement();
#endif // WIREFRAME_USE_NATIVE_BUFFERS
    return mesh;
}

bool IsWireframeReady(WireframeMesh wireframe_mesh) {
    int res = (
        wireframe_mesh.vertex_count * wireframe_mesh.line_count * 
        (size_t)wireframe_mesh.vertecies * (size_t)wireframe_mesh.vertex_distances * 
        (size_t)wireframe_mesh.lines
    );  // >;)
    return res;
}

void UnLoadWireframeMesh(WireframeMesh wireframe_mesh) {
#ifdef WIREFRAME_USE_NATIVE_BUFFERS
    rlUnloadVertexArray(wireframe_mesh.vao);
    rlUnloadVertexBuffer(wireframe_mesh.vbo_vertecies);
    rlUnloadVertexBuffer(wireframe_mesh.vbo_lines);
#endif // WIREFRAME_USE_NATIVE_BUFFERS

    delete[] wireframe_mesh.vertecies;
    delete[] wireframe_mesh.vertex_distances;
    delete[] wireframe_mesh.lines;
}