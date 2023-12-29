#ifndef BASIC_H
#define BASIC_H

#include <raylib.h>
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

//#include "entt.hpp"
typedef uint32_t entity_id_t;
constexpr entity_id_t GetInvalidId() { return UINT32_MAX; }

#endif // BASIC_H