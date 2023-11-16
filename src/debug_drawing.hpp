#ifndef DEBUG_DRAWING_H
#define DEBUG_DRAWING_H

#include "basic.hpp"

// Draws construction directly in astronomical coordinate system

void DebugDrawLine(Vector2 from, Vector2 to);
void DebugDrawConic(Vector2 focus, Vector2 ecc_vector, double a);

void DebugFlushText();
void DebugPrintText(const char* format, ...);

#define DEBUG_SHOW_F(var) DebugPrintText("%s = %f", #var, var);
#define DEBUG_SHOW_I(var) DebugPrintText("%s = %d", #var, var);

#endif // DEBUG_DRAWING_H