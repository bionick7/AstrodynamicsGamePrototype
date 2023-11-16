#include "debug_drawing.hpp"
#include "global_state.hpp"
#include "coordinate_transform.hpp"
#include "ui.hpp"

void DebugDrawLine(Vector2 from, Vector2 to) {
    const CoordinateTransform* c_transf = GetScreenTransform();
    DrawLineV(
        c_transf->TransformV(from),
        c_transf->TransformV(to),
        GREEN
    );
}

Vector2 point_buffer[64];

void DebugDrawConic(Vector2 focus, Vector2 ecc_vector, double a) {
    const CoordinateTransform* c_transf = GetScreenTransform();
    Vector2 x = Vector2Normalize(ecc_vector);
    Vector2 y = Vector2Rotate(x, PI/2);
    double e = Vector2Length(ecc_vector);
    //SHOW_F(a) SHOW_F(e)
    //double b = e > 1 ? a * sqrt(e - 1) : a * sqrt(1 - e);
    double p = a*(1 - e*e);
    for (int i=0; i < 64; i++) {
        double theta = 2*PI * i / 64.0;
        double r = p / (1 + e*cos(theta));
        //if (r < 0) r = 0;
        point_buffer[i] = Vector2Add(focus, Vector2Add(
            Vector2Scale(x, r*cos(theta)),
            Vector2Scale(y, r*sin(theta))
        ));
    }
    c_transf->TransformBuffer(&point_buffer[0], 64);
    DrawLineV(
        c_transf->TransformV(focus),
        c_transf->TransformV(Vector2Add(focus, Vector2Scale(x, -2*a*e))),
        GREEN
    );
    DrawLineStrip(&point_buffer[0], 64, GREEN);
}

char lines[64][256];
int lines_index = 0;

void DebugFlushText() {
    TextBox debug_textbox = TextBox(5, 35, 500, GetScreenHeight() - 40, 16, GREEN);
    debug_textbox.text_background = BLACK;
    for (int i=0; i < lines_index; i++) {
        debug_textbox.WriteLine(lines[i]);
    }
    lines_index = 0;
}

void DebugPrintText(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(lines[lines_index], format, args);
    va_end(args);
    
    if (lines_index >= 63) return;
    lines_index++;
}