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

#define BG_COLOR          GetColor(0x1D2025FFu)
#define PALETTE_BLUE      GetColor(0x006D80FFu)
#define PALETTE_GREEN     GetColor(0x4Fc76CFFu)
#define PALETTE_RED       GetColor(0xFF5155FFu)
#define PALETTE_WHITE     GetColor(0xFFFFA0FFu)

#define MAIN_UI_COLOR     PALETTE_WHITE
#define TRANSFER_UI_COLOR PALETTE_GREEN

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

const double G = 6.6743015e-11;  // m³/(s²kg)
static inline Vector2 GetScreenCenter() { return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}; }

typedef double time_type;

void FormatTime(char* buffer, int buffer_len, time_type time);
void FormatDate(char* buffer, int buffer_len, time_type time);

#endif // BASIC_H