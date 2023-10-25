#ifndef BASIC_H
#define BASIC_H

#include <raylib.h>
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
static inline Vector2 GetScreenCenter() { return (Vector2) {GetScreenWidth() / 2, GetScreenHeight() / 2}; }

#define STRUCT_DECL(name) typedef struct s_##name name;  struct s_##name 
#define ENUM_DECL(name) typedef enum e_##name name;  enum e_##name 
typedef double time_type;

#define ASSERT(condition) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s)\n", __FILE__, __LINE__, #condition); }
#define ASSERT_MSG(condition, msg) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s) with message %s\n", __FILE__, __LINE__, #condition, msg); }
#define ASSERT_ALOMST_EQUAL(v1, v2) if (fabs(v1 - v2) > v1 * 1e-5) { printf("%s:%d :: Assertion failed: %s (%f) != %s (%f)\n", __FILE__, __LINE__, #v1, v1, #v2, v2); }

#define NOT_IMPLEMENTED printf("Not Implemented %s:%d:1\n", __FILE__, __LINE__); exit(1);
#define FAIL(msg) printf("Runtime Error %s:%d:1 :: %s\n", __FILE__, __LINE__, msg); exit(1);
#define FAIL_FORMAT(msg, ...) {\
    char error_msg[1024]; \
    snprintf(error_msg, 1024, msg, __VA_ARGS__); \
    printf("Runtime Error %s:%d:1 :: %s\n", __FILE__, __LINE__, error_msg); \
    exit(1);}

#define SHOW_F(var) printf("%s:%d :: %s = %f\n", __FILE__, __LINE__, #var, var);
#define SHOW_I(var) printf("%s:%d :: %s = %d\n", __FILE__, __LINE__, #var, var);
#define SHOW_V2(var) printf("%s:%d :: %s = (%f, %f)\n", __FILE__, __LINE__, #var, (var).x, (var).y);

void ForamtTime(char* buffer, int buffer_len, time_type time);

#endif // BASIC_H