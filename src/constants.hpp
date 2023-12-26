#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "basic.hpp"

namespace Palette {
    extern Color bg;
    extern Color blue;
    extern Color green;
    extern Color red;
    extern Color white;
    extern Color ui_main;
    extern Color ui_dark;
    extern Color ui_alt;
    extern Color interactable_main;
    extern Color interactable_dark;
    extern Color interactable_alt;
    extern Color ship;
    extern Color ship_dark;
    extern Color ship_alt;
}

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

const double G = 6.6743015e-11;  // m³/(s²kg)

#endif  // CONSTANTS_H