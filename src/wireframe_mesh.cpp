#include "wireframe_mesh.hpp"
#include "rlgl.h"
#include "logging.hpp"

WireframeMesh _MakeWireframeMeshFromBuffers(int p_vertex_count, int p_triangle_count, int p_line_count,
    float* vertex_buffer, float* color_buffer, unsigned short* triangle_buffer, unsigned short* line_buffer
) {
    WireframeMesh mesh = WireframeMesh {0};

    mesh.vertex_count = p_vertex_count;
    mesh.triangle_count = p_triangle_count;
    mesh.line_count = p_line_count;

    // Triangles
    mesh.vao_triangles = rlLoadVertexArray();
    rlEnableVertexArray(mesh.vao_triangles);

    mesh.vbo_triangles = rlLoadVertexBuffer(vertex_buffer, p_vertex_count*3*sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(0);
    
    mesh.vbo_triangle_colors = rlLoadVertexBuffer(color_buffer, p_vertex_count*3*sizeof(float), false);
    rlSetVertexAttribute(2, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(2);

    int triangle_elements = rlLoadVertexBufferElement(triangle_buffer, p_triangle_count*3*sizeof(unsigned short), false);
    rlDisableVertexArray();
        
    // Lines
    mesh.vao_lines = rlLoadVertexArray();
    rlEnableVertexArray(mesh.vao_lines);

    mesh.vbo_lines = rlLoadVertexBuffer(vertex_buffer, p_vertex_count*3*sizeof(float), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(0);

    int line_elements = rlLoadVertexBufferElement(line_buffer, p_line_count*2*sizeof(unsigned short), false);
    rlDisableVertexArray();

    return mesh;
}

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
    char* text = LoadFileText(filepath);
    if (text == NULL) {
        return {0};
    }
    int text_size = strlen(text);
    int text_cursor = 0;

    int read_bytes = 0;

    int counter = 0;
    int line = 0;

    int vertex_count = 0;
    int line_count = 0;
    int triangle_count = 0;

    do {
        line++;
        char c = text[text_cursor];
        char c1 = text_cursor < text_size - 1 ? text[text_cursor + 1] : '\0';
        while (c <= 32 && c != '\n' && c != '\0') {
            c = text[++text_cursor];
        }
        //INFO("%d => '%c'(%u)", line, c,c)
        if (c == '\n' || c == '\0') continue;
        
        if (c == '#') continue;
        else if (c == 'v' && c1 == ' ') vertex_count++;
        else if (c == 'l' && c1 == ' ') line_count++;
        else if (c == 'f' && c1 == ' ') triangle_count++;
        else if (c == 'v' && c1 == 't') continue;  // Ignore tangents
        else if (c == 'v' && c1 == 'n') continue;  // Ignore normals
        else if (c == 'o' && c1 == ' ') continue;  // Ignore object name
        else if (c == 's' && c1 == ' ') continue;  // Ignore surfaces
        else if (text_size - text_cursor >= 6 && strncmp("mtllib", &text[text_cursor], 6) == 0) ;  // Ignore material library
        else ERROR("Unexpected character ('%c'%u) when reading at %s:%d:0", c, c, filepath, line)
    } while (_GetNextLine(text, &text_cursor));

    float* vertices = new float[vertex_count*3];
    float* colors = new float[vertex_count*3];
    float* vertex_distances = new float[vertex_count];
    unsigned short* lines = new unsigned short[line_count*2];
    unsigned short* triangles = new unsigned short[triangle_count*3];

    int vertex_cursor = 0;
    int line_cursor = 0;
    int triangle_cursor = 0;

    BoundingBox bounding_box;
    bounding_box.min = { INFINITY, INFINITY, INFINITY };
    bounding_box.max = {-INFINITY,-INFINITY,-INFINITY };

    text_cursor = 0;
    line = 0;
    do {
        line++;
        if (text[text_cursor] == 'v' && text[text_cursor + 1] == ' ') {
            float x, y, z;
            float nx, ny, nz;
            int scanfres = sscanf(&text[text_cursor], "v %f %f %f %f %f %f", &x, &y, &z, &nx, &ny, &nz);
            if (scanfres != 6) {
                scanfres = sscanf(&text[text_cursor], "v %f %f %f", &x, &y, &z);
                nx = 0; ny = 1; nz = 0;
                if (scanfres != 3) {
                    ERROR("Could not read vertex at %s:%d", filepath, line)
                }
            }
            vertices[3*vertex_cursor] = x;
            vertices[3*vertex_cursor+1] = y;
            vertices[3*vertex_cursor+2] = z;
            colors[3*vertex_cursor] = nx;
            colors[3*vertex_cursor+1] = ny;
            colors[3*vertex_cursor+2] = nz;

            if (x > bounding_box.max.x) bounding_box.max.x = x;
            if (x < bounding_box.min.x) bounding_box.min.x = x;
            if (y > bounding_box.max.y) bounding_box.max.y = y;
            if (y < bounding_box.min.y) bounding_box.min.y = y;
            if (z > bounding_box.max.z) bounding_box.max.z = z;
            if (z < bounding_box.min.z) bounding_box.min.z = z;

            if (vertex_cursor == 0) {
                vertex_distances[vertex_cursor] = 0;
            } else {
                float dx = x - vertices[3 * vertex_cursor - 3];
                float dy = y - vertices[3 * vertex_cursor - 2];
                float dz = z - vertices[3 * vertex_cursor - 1];
                float distance_to_previous = sqrtf(dx*dx + dy*dy + dz*dz);
                vertex_distances[vertex_cursor] = vertex_distances[vertex_cursor-1] + distance_to_previous;
            }
            //INFO("%d - %f", vertex_cursor, vertex_distances[vertex_cursor])

            vertex_cursor++;
        }
        else if (text[text_cursor] == 'l' && text[text_cursor + 1] == ' ') {
            int l1, l2;
            int scanfres = sscanf(&text[text_cursor], "l %d %d", &l1, &l2);
            if (scanfres != 2) {
                ERROR("Could not read line-segment at %s:%d", filepath, line)
            }
            lines[2*line_cursor] = l1;
            lines[2*line_cursor+1] = l2;
            line_cursor++;
        }
        else if (text[text_cursor] == 'f' && text[text_cursor + 1] == ' ') {
            int fs1, fs2, fs3;
            int scanfres = sscanf(&text[text_cursor], "f %d %d %d", &fs1, &fs2, &fs3);
            if (scanfres != 3) {
                ERROR("Could not read face at %s:%d", filepath, line)
            }
            triangles[3*triangle_cursor] = fs1;
            triangles[3*triangle_cursor+1] = fs2;
            triangles[3*triangle_cursor+2] = fs3;
            //INFO("faces: %d, %d, %d\n", fs1, fs2, fs3);
            triangle_cursor++;
        }
    } while (_GetNextLine(text, &text_cursor));
    if (triangle_cursor != triangle_count || line_cursor != line_count || vertex_cursor != vertex_count) {
        FAIL("Mismatch while reading mesh '%s' tris: %d/%d, lines: %d/%d, verts: %d/%d)", 
            filepath, triangle_cursor, triangle_count, line_cursor, line_count, vertex_cursor, vertex_count)
    }

    UnloadFileText(text);

    // Colorize vertex distances
    float total_distance = vertex_distances[vertex_count-1];
    for(int i=0; i < vertex_count; i++) {
        vertex_distances[i] = vertex_distances[i] / total_distance;
    }

    // .obj starts indices at 1, not 0
    for(int i=0; i < line_count*2; i++) lines[i]--;
    for(int i=0; i < triangle_count*3; i++) triangles[i]--;

    WireframeMesh mesh = _MakeWireframeMeshFromBuffers(
        vertex_count, triangle_count, line_count,
        vertices, colors, triangles, lines
    );

    delete[] vertices;
    delete[] colors;
    delete[] vertex_distances;
    delete[] lines;
    delete[] triangles;

    mesh.bounding_box = bounding_box;

    return mesh;
}

WireframeMesh LoadTestWireframeMesh() {
    static float vertex_buffer[4*3] = {
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
    };

    static float color_buffer[4*3] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    static unsigned short triangle_index_buffer[6] = {0, 1, 2, 3, 1, 0};
    static unsigned short line_index_buffer[8] = {0, 3, 3, 1, 1, 2, 2, 0};

    WireframeMesh mesh = _MakeWireframeMeshFromBuffers(4, 4, 2, vertex_buffer, 
        color_buffer, triangle_index_buffer, line_index_buffer);
    mesh.bounding_box.min = {0,0,0};
    mesh.bounding_box.max = {1,0,1};
    return mesh;
}

bool IsWireframeReady(WireframeMesh wireframe_mesh) {
    int res = (
        wireframe_mesh.vertex_count * (wireframe_mesh.line_count + wireframe_mesh.triangle_count) *
        (wireframe_mesh.vao_triangles + wireframe_mesh.vao_lines) * 
        (wireframe_mesh.vbo_triangles + wireframe_mesh.vbo_lines)
    );  // >;)
    return res;
}

void UnloadWireframeMesh(WireframeMesh wireframe_mesh) {
    rlUnloadVertexArray(wireframe_mesh.vao_triangles);
    rlUnloadVertexArray(wireframe_mesh.vao_lines);
    rlUnloadVertexBuffer(wireframe_mesh.vbo_triangles);
    rlUnloadVertexBuffer(wireframe_mesh.vbo_lines);

    wireframe_mesh.vao_triangles = -1;
    wireframe_mesh.vao_lines = -1;
    wireframe_mesh.vbo_triangles = -1;
    wireframe_mesh.vbo_lines = -1;

    wireframe_mesh.vertex_count = 0;
    wireframe_mesh.line_count = 0;
    wireframe_mesh.triangle_count = 0;

    wireframe_mesh.bounding_box = {0};
}