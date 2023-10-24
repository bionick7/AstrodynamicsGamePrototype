#ifndef UTILS_H
#define UTILS_H

#include "basic.h"

int ClampInt(int x, int min, int max);
double LerpFromArray(double t, double array[], int array_len);
double Sign(double in);
Vector2 FromPolar(double radius, double phase);
Vector2 Apply2DTransform(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
Vector2 Apply2DTransformInv(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
double Determinant(Vector2 column1, Vector2 column2);
void Swap(void* lhs, void* rhs);
double PosMod(double x, double period);

#endif  // UTILS_H