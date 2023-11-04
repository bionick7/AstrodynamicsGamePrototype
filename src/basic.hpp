#ifndef BASIC_H
#define BASIC_H

#include <raylib.h>
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "entt.hpp"
typedef entt::entity entity_id_t;

/*
#242D36
#004B61
#006D80
#00908A
#00B17C
#00D056
*/

#define BG_COLOR          GetColor(0x1D2025FFu)
#define PALETTE_BLUE      GetColor(0x006D80FFu)
#define PALETTE_GREEN     GetColor(0x4Fc76CFFu)
#define MAIN_UI_COLOR     GetColor(0xFFFFA0FFu)
#define TRANSFER_UI_COLOR GetColor(0xFF5155FFu)

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
static inline Vector2 GetScreenCenter() { return (Vector2) {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }

typedef double time_type;

#define DEBUG

#ifdef DEBUG

#define ASSERT(condition) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s)\n", __FILE__, __LINE__, #condition); }
#define ASSERT_MSG(condition, msg) if (!(condition)) { printf("%s:%d :: Assertion failed: (%s) with message %s\n", __FILE__, __LINE__, #condition, msg); }
#define ASSERT_ALOMST_EQUAL(v1, v2) if (fabs(v1 - v2) > v1 * 1e-5) { printf("%s:%d :: Assertion failed: %s (%f) != %s (%f)\n", __FILE__, __LINE__, #v1, v1, #v2, v2); }

#define NOT_IMPLEMENTED {printf("Not Implemented %s:%d:1\n", __FILE__, __LINE__); exit(1);}
#define FAIL(msg) {printf("Runtime Error %s:%d:1 :: %s\n", __FILE__, __LINE__, msg); exit(1);}
#define FAIL_FORMAT(msg, ...) {\
    char error_msg[1024]; \
    snprintf(error_msg, 1024, msg, __VA_ARGS__); \
    printf("Runtime Error %s:%d:1 :: %s\n", __FILE__, __LINE__, error_msg); \
    exit(1);}

#define SHOW_F(var) printf("%s:%d :: %s = %f\n", __FILE__, __LINE__, #var, var);
#define SHOW_I(var) printf("%s:%d :: %s = %d\n", __FILE__, __LINE__, #var, var);
#define SHOW_V2(var) printf("%s:%d :: %s = (%f, %f)\n", __FILE__, __LINE__, #var, (var).x, (var).y);

#else

#define ASSERT(condition) 
#define ASSERT_MSG(condition, msg) 
#define ASSERT_ALOMST_EQUAL(v1, v2) 
#define NOT_IMPLEMENTED
#define FAIL(msg)
#define FAIL_FORMAT(msg, ...) 
#define SHOW_F(var)
#define SHOW_I(var)
#define SHOW_V2(var)

#endif  // DEBUG

void FormatTime(char* buffer, int buffer_len, time_type time);
void FormatDate(char* buffer, int buffer_len, time_type time);

#endif // BASIC_H