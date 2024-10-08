#ifndef UTILS_H
#define UTILS_H

#include "basic.hpp"
#include "dvector3.hpp"

int MinInt(int a, int b);
int MaxInt(int a, int b);
int ClampInt(int x, int min, int max);
double LerpFromArray(double t, double array[], int array_len);
double Sign(double in);
Vector2 FromPolar(double radius, double phase);
Vector2 Apply2DTransform(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
Vector2 Apply2DTransformInv(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp);
double Determinant(Vector2 column1, Vector2 column2);
Vector2 Vector2Project(Vector2 v, Vector2 projected);
//void Swap(void* lhs, void* rhs);
double PosMod(double x, double period);
double Smoothstep(double a, double b, double x);
float Smoothstep(float a, float b, float x);

Rectangle EncapsulationRectangle(Rectangle a, Rectangle b);
bool CheckEnclosingRecs(Rectangle outside, Rectangle inside);

Matrix MatrixFromColumns(Vector3 col_x, Vector3 col_y, Vector3 col_z);
Matrix MatrixFromColumns(Vector3 col_x, Vector3 col_y, Vector3 col_z, Vector3 origin);

int FindInArray(const char* const search[], int search_count, const char* identifier);

namespace randomgen {
    void SetRandomSeed(uint_fast64_t seed);
    double GetRandomUniform(double from, double to);
    double GetRandomGaussian(double mean, double std);
    DVector3 RandomOnSphere();
}

#endif  // UTILS_H