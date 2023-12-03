#include "utils.hpp"
#include <random>


int MinInt(int a, int b) {
    return a < b ? a : b;
}

int MaxInt(int a, int b) {
    return a > b ? a : b;
}

int ClampInt(int x, int min, int max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

double Sign(double in) {
    //return ((unsigned long)((void*) &in) << 63 == 1) ? 1 : -1;
    return in > 0 ? 1 : -1;
}

double LerpFromArray(double t, double array[], int array_len) {
    int index_left = floorf(t * array_len);
    if (index_left < 0) index_left = 0;
    if (index_left >= array_len - 1) index_left = array_len - 2;
    int index_right = index_left + 1;
    double t_local = (t * array_len - index_left) / (index_right - index_left);
    if (t_local < 0.0) t_local = 0.0;
    if (t_local > 1.0) t_local = 1.0;
    return array[index_left] * t_local + array[index_right] * (1.0 - t_local);
}

Vector2 FromPolar(double radius, double phase) {
    return {
        radius * cos(phase),
        radius * sin(phase)
    };
}

Vector2 Apply2DTransform(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp) {
    return Vector2Add(origin,
        Vector2Add(
            Vector2Scale(x, inp.x),
            Vector2Scale(y, inp.y)
        )
    );
}

Vector2 Apply2DTransformInv(Vector2 origin, Vector2 x, Vector2 y, Vector2 inp) {
    // x = Ay + b
    // y = A⁻¹ (x - b)
    Vector2 solution = Vector2Subtract(inp, origin);
    return Vector2Scale(Apply2DTransform({0}, {y.y, -x.y}, {-y.x, x.x}, solution), 1. / Determinant(x, y));
}

double Determinant(Vector2 column1, Vector2 column2) {
    return column1.x * column2.y - column1.y * column2.x;
}
/*
void Swap(void* lhs, void* rhs) {
    void* tmp = rhs;
    rhs = lhs;
    lhs = tmp;
}*/

double PosMod(double x, double period) {
    double res = fmod(x, period);
    if (res < 0) res += period;
    return res;
}

static std::mt19937_64 r_generator = std::mt19937_64(std::random_device{}());

double SetRandomSeed(uint_fast64_t seed) {
    r_generator.seed(seed);
}

double GetRandomUniform(double from, double to) {
    auto uniform = std::uniform_real_distribution(from, to);
    return uniform(r_generator);
}

double GetRandomGaussian(double mean, double std) {
    auto normal = std::normal_distribution(mean, std);
    return normal(r_generator);
}