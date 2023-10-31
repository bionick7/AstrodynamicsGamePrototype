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

TextBox debug_textbox;
char lines[64][256];
int lines_index = 0;

void DebugFlushText() {
    debug_textbox = TextBoxMake(5, 35, 500, GetScreenHeight() - 40, 16, GREEN);
    debug_textbox.text_background = BLACK;
    for (int i=0; i < lines_index; i++) {
        TextBoxWriteLine(&debug_textbox, lines[i]);
    }
    lines_index = 0;
}

void DebugPrintText(const char* text) {
    if (lines_index >= 63) return;
    strcpy(lines[lines_index], text);
    lines_index++;
}

void DebugPrintVarF(const char* var_name, float var) {
    size_t varname_len = strlen(var_name);
    if (varname_len + 13 >= 256) {
        return;
    }
    char res[256];
    strcpy(res, var_name);
    sprintf(&res[varname_len], "%10f", var);
    DebugPrintText(res);
}

void DebugPrintVarI(const char* var_name, int var) {
    int varname_len = strlen(var_name);
    if (varname_len + 13 >= 256) {
        return;
    }
    char res[256];
    strcpy(res, var_name);
    sprintf(&res[varname_len], "%10d", var);
    DebugPrintText(res);
}