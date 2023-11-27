#ifndef UTILS_H
#define UTILS_H

#include "basic.hpp"

int MinInt(int a, int b);
int MaxInt(int a, int b);
int ClampInt(int x, int min, int max);
double LerpFromArray(double t, double array[], int array_len);
double Sign(double in);
Vector2 FromPolar(double radius, double phase);
Vector2 Apply2DTransform(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
Vector2 Apply2DTransformInv(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
double Determinant(Vector2 column1, Vector2 column2);
//void Swap(void* lhs, void* rhs);
double PosMod(double x, double period);

double SetRandomSeed(uint_fast64_t seed);
double GetRandomUniform(double from, double to);
double GetRandomGaussian(double mean, double std);

#endif  // UTILS_H