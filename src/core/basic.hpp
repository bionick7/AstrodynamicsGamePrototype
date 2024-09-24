#ifndef BASIC_H
#define BASIC_H

#include <raylib.h>
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <vector>

#if __WORDSIZE == 64
    #define LONG_STRID "l"
#else
    #define LONG_STRID "ll"
#endif

typedef uint64_t StrHash;
StrHash HashKey(const char* key);

#endif // BASIC_H