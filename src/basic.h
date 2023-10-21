#ifndef BASIC_H
#define BASIC_H

#include <raylib.h>
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
#define SCREEN_CENTER (Vector2) { SCREEN_WIDTH/2, SCREEN_HEIGHT/2 }

typedef double time_type;

#define ASSERT(condition) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s)\n", __FILE__, __LINE__, #condition); }
#define ASSERT_MSG(condition, msg) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s) with message %s\n", __FILE__, __LINE__, #condition, msg); }
#define ASSERT_ALOMST_EQUAL(v1, v2) if (fabs(v1 - v2) > v1 * 1e-5) { printf("%s:%d :: Assertion failed: %s (%f) != %s (%f)\n", __FILE__, __LINE__, #v1, v1, #v2, v2); }

#define NOT_IMPLEMENTED printf("Not Implemented %s:%d:1\n", __FILE__, __LINE__); exit(1);
#define FAIL(msg) printf("Runtime Error %s:%d:1 :: %s\n", __FILE__, __LINE__, msg); exit(1);

#define SHOW_F(var) printf("%s:%d :: %s = %f\n", __FILE__, __LINE__, #var, var);
#define SHOW_I(var) printf("%s:%d :: %s = %d\n", __FILE__, __LINE__, #var, var);

void swap(void* lhs, void* rhs);

#endif // BASIC_H