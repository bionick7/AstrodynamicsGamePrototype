#include "utils.hpp"
#include "logging.hpp"
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
        (float) (radius * cos(phase)),
        (float) (radius * sin(phase))
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

Vector2 Vector2Project(Vector2 v, Vector2 onto) {
    if (onto.x*onto.y == 0) {
        return Vector2Zero();
    }
    return Vector2Scale(onto, Vector2DotProduct(v, onto) / Vector2LengthSqr(onto));
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

double Smoothstep(double a, double b, double t) {
    if (b < a) return 1 - Smoothstep(b, a, t);
    if (t <= a) return 0;
    if (t >= b) return 1;
    double x = (t - a) / (b - a);
    return x*x*(3 - 2*x);
}

float Smoothstep(float a, float b, float t) {
    if (b < a) return 1 - Smoothstep(b, a, t);
    if (t <= a) return 0;
    if (t >= b) return 1;
    float x = (t - a) / (b - a);
    return x*x*(3 - 2*x);
}

// Generates smallest possible rectange that includes a and b
Rectangle EncapsulationRectangle(Rectangle a, Rectangle b) {
    if (b.x < a.x) a.x = b.x;
    if (b.x < a.x) a.x = b.x;
    if (b.x + b.width > a.x + a.width) a.width = b.x + b.width - a.x;
    if (b.y + b.height > a.y + a.height) a.height = b.y + b.height - a.y;
    return a;
}

// Returns true if 'outside' fully encloses 'inside'
bool CheckEnclosingRecs(Rectangle outside, Rectangle inside) {
    return
        outside.x < inside.x && outside.x + outside.width  > inside.x + inside.width &&
        outside.y < inside.y && outside.y + outside.height > inside.y + inside.height
    ;
}

Matrix MatrixFromColumns(Vector3 col_x, Vector3 col_y, Vector3 col_z, Vector3 origin) {
    Matrix m = {0};
    m.m0  = col_x.x;
    m.m1  = col_x.y;
    m.m2  = col_x.z;
    m.m4  = col_y.x;
    m.m5  = col_y.y;
    m.m6  = col_y.z;
    m.m8  = col_z.x;
    m.m9  = col_z.y;
    m.m10 = col_z.z;
    m.m12 = origin.x;
    m.m13 = origin.y;
    m.m14 = origin.z;
    m.m15 = 1.0f;
    return m;
}

Matrix MatrixFromColumns(Vector3 col_x, Vector3 col_y, Vector3 col_z) {
    return MatrixFromColumns(col_x, col_y, col_z, Vector3Zero());
}

int FindInArray(const char* const search[], int search_count, const char* identifier) {
    int res = -1;
    for (int i=0; i < search_count; i++){
        if (strcmp(identifier, search[i]) == 0) {
            res = i;
            break;
        }
    }
    if (res < 0) {
        ERROR("'%s' not found in respective array ('%s', ...)", identifier, search)
    }
    return res;
}

static std::mt19937_64 r_generator = std::mt19937_64(std::random_device{}());

void randomgen::SetRandomSeed(uint_fast64_t seed) {
    r_generator.seed(seed);
}

double randomgen::GetRandomUniform(double from, double to) {
    auto uniform = std::uniform_real_distribution(from, to);
    return uniform(r_generator);
}

double randomgen::GetRandomGaussian(double mean, double std) {
    auto normal = std::normal_distribution(mean, std);
    return normal(r_generator);
}

DVector3 randomgen::RandomOnSphere() {
    // A = 2pi cos(x) dx
    // A_tot = 4pi
    // PDF = cos(x)/2
    // CDF = (sin(x) + 1)/2
    double azimuth = GetRandomUniform(0, 2*PI);
    double elevation = asin(2 * GetRandomUniform(0, 1) - 1);
    return DVector3(
        cos(azimuth) * cos(elevation),
        sin(azimuth) * cos(elevation),
        sin(elevation)
    );
}