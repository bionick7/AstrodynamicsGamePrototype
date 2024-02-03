#include "debug_drawing.hpp"
#include "global_state.hpp"
#include "coordinate_transform.hpp"
#include "ui.hpp"
#include "constants.hpp"

#include "rlgl.h"

#define MAX_LINE_BUFFER 1024*128
static Vector3 line_buffer[MAX_LINE_BUFFER];
int line_buffer_index;

void DebugDrawLineRenderSpace(Vector3 from, Vector3 to) {
    if (line_buffer_index >= line_buffer_index + 1)
        return;
    line_buffer[line_buffer_index++] = from;
    line_buffer[line_buffer_index++] = to;
}

void DebugDrawLine(DVector3 from, DVector3 to) {
    DebugDrawLineRenderSpace(
        GameCamera::WorldToRender(from),
        GameCamera::WorldToRender(to)
    );
}

void DebugDrawTransform(Matrix mat) {
    Vector3 origin = {mat.m12, mat.m13, mat.m14};
    DebugDrawLineRenderSpace(origin, Vector3Add(origin, {mat.m0, mat.m1, mat.m2}));
    DebugDrawLineRenderSpace(origin, Vector3Add(origin, {mat.m4, mat.m5, mat.m6}));
    DebugDrawLineRenderSpace(origin, Vector3Add(origin, {mat.m8, mat.m9, mat.m10}));
}

Vector3 point_buffer[64];

void DebugDrawConic(DVector3 focus, DVector3 ecc_vector, DVector3 normal, double sma) {
    normal = normal.Normalized();
    DVector3 x = ecc_vector.Normalized();
    DVector3 y = x.Rotated(normal, PI/2);
    double e = ecc_vector.Length();
    //SHOW_F(sma) SHOW_F(e)
    //double b = e > 1 ? a * sqrt(e - 1) : a * sqrt(1 - e);
    double p = sma*(1 - e*e);
    DVector3 prev = DVector3::Zero();
    for (int i=0; i < 64; i++) {
        double theta = 2*PI * i / 64.0;
        double r = p / (1 + e*cos(theta));
        //if (r < 0) r = 0;
        DVector3 pt = (focus + r * cos(theta) * x + r * sin(theta) * y);
        if (i != 0) {
            DebugDrawLine(prev, pt);
            prev = pt;
        }
    }
    DebugDrawLine(focus, focus - x * 2*sma*e);
}

char lines[64][256];
int lines_index = 0;

void DebugFlushText() {
    TextBox debug_textbox = TextBox(5, 35, 500, GetScreenHeight() - 40, 16, Palette::green);
    debug_textbox.text_background = BLACK;
    for (int i=0; i < lines_index; i++) {
        debug_textbox.WriteLine(lines[i]);
    }
    lines_index = 0;
}

void DebugFlush3D() {
    rlBegin(RL_LINES);
    rlColor4ub(0, 255, 0, 255);
    for(int i=0; i < line_buffer_index; i++) {
        rlVertex3f(line_buffer[i].x, line_buffer[i].y, line_buffer[i].z);
    }
    rlEnd();
    line_buffer_index = 0;
}

void DebugPrintText(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(lines[lines_index], format, args);
    va_end(args);
    
    if (lines_index >= 63) return;
    lines_index++;
}