#ifndef DEBUG_DRAWING_H
#define DEBUG_DRAWING_H

#include "basic.hpp"
#include "dvector3.hpp"

// Draws construction directly in astronomical coordinate system

void DebugDrawLineRenderSpace(Vector3 from, Vector3 to);
void DebugDrawLine(DVector3 from, DVector3 to);
void DebugDrawTransform(Matrix mat);
void DebugDrawConic(DVector3 focus, DVector3 ecc_vector, DVector3 normal, double a);

void DebugFlushText();
void DebugFlush3D();
void DebugPrintText(const char* format, ...);

#define DEBUG_SHOW_F(var) DebugPrintText("%s = %f", #var, var);
#define DEBUG_SHOW_I(var) DebugPrintText("%s = %d", #var, var);

#endif // DEBUG_DRAWING_H