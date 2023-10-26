#include "debug_drawing.h"
#include "global_state.h"
#include "camera.h"
#include "ui.h"

void DebugDrawLine(Vector2 from, Vector2 to) {
    const DrawCamera* cam = &GlobalGetState()->camera;
    DrawLineV(
        CameraTransformV(cam, from),
        CameraTransformV(cam, to),
        GREEN
    );
}

Vector2 point_buffer[64];

void DebugDrawConic(Vector2 focus, Vector2 ecc_vector, double a) {
    const DrawCamera* cam = &GlobalGetState()->camera;
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
    CameraTransformBuffer(cam, &point_buffer[0], 64);
    DrawLineV(
        CameraTransformV(cam, focus),
        CameraTransformV(cam, Vector2Add(focus, Vector2Scale(x, -2*a*e))),
        GREEN
    );
    DrawLineStrip(&point_buffer[0], 64, GREEN);
}

TextBox debug_textbox;

void DebugClearText() {
    debug_textbox = TextBoxMake(5, 35, 500, GetScreenHeight() - 40, 16, GREEN);
}

void DebugPrintText(const char* text) {
    TextBoxWriteLine(&debug_textbox, text);
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