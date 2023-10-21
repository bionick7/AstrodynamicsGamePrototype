#ifndef DEBUG_DRAWING_H
#define DEBUG_DRAWING_H

#include "basic.h"

// Draws construction directly in astronomical coordinate system

void DebugDrawLine(Vector2 from, Vector2 to);
void DebugDrawConic(Vector2 focus, Vector2 ecc_vector, double a);

void DebugClearText();
void DebugPrintText(const char* text);
void DebugPrintVarF(const char* var_name, float var);
void DebugPrintVarI(const char* var_name, int var);

#define DEBUG_SHOW_F(var) DebugPrintVarF(#var, var);
#define DEBUG_SHOW_I(var) DebugPrintVarI(#var, var);

#endif // DEBUG_DRAWING_H