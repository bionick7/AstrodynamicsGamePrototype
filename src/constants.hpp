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
    extern Color ally;
    extern Color ally_dark;
    extern Color ally_alt;
    extern Color enemy;
    extern Color enemy_dark;
    extern Color enemy_alt;
}

#define ICON_EMPTY "\u04ff"  // lower-right corner
#define ICON_PLANET "\u0300"

#define ICON_WATER           "\u0320"
#define ICON_HYDROGEN        "\u0321"
#define ICON_OXYGEN          "\u0322"
#define ICON_ROCK            "\u0324"
#define ICON_IRON_ORE        "\u0323"
#define ICON_STEEL           "\u0325"
#define ICON_FOOD            "\u0326"
#define ICON_BIOMASS         "\u0327"
#define ICON_WASTE           "\u0328"
#define ICON_CO2             "\u0329"
#define ICON_CARBON          "\u032A"
#define ICON_POLYMERS        "\u032B"
#define ICON_ELECTRONICS     "\u032C"
#define ICON_ALUMINIUM       "\u032D"
#define ICON_ALUMINIUM_ORE   "\u032E"

#define ICON_HEART           "\u0400"
#define ICON_HEART_ORDNANCE  "\u0401"
#define ICON_HEART_ENERGY    "\u0402"
#define ICON_HEART_BOARDING  "\u0403"
#define ICON_HEART_KINETIC   "\u0404"

#define ICON_SHIELD          "\u0420"
#define ICON_SHIELD_ORDNANCE "\u0421"
#define ICON_SHIELD_ENERGY   "\u0422"
#define ICON_SHIELD_BOARDING "\u0423"
#define ICON_SHIELD_KINETIC  "\u0424"
#define ICON_PAYLOAD         "\u0425"
#define ICON_POWER           "\u0426"

#define ICON_ATTACK          "\u0440"
#define ICON_ATTACK_ORDNANCE "\u0441"
#define ICON_ATTACK_ENERGY   "\u0442"
#define ICON_ATTACK_BOARDING "\u0443"
#define ICON_ATTACK_KINETIC  "\u0444"
#define ICON_DV              "\u0445"
#define ICON_MASS            "\u0446"

#define ICON_CORE            "\u0460"
#define ICON_GUN             "\u0461"
#define ICON_DEPLOY          "\u0462"
#define ICON_SAIL            "\u0463"
#define ICON_SYSTEM          "\u0464"
#define ICON_SIZE            "\u0467"

#define ICON_UTIL_SHIP       "\u0480"
#define ICON_STATION         "\u0481"
#define ICON_TRANSPORT_SHIP  "\u0482"
#define ICON_MIL_SHIP        "\u0483"
#define ICON_TRANSPORT_FLEET "\u0484"
#define ICON_MIL_FLEET       "\u0485"

#define ICON_CROSSHAIR       "\u04a0"
#define ICON_LOOKINGGLASS    "\u04a1"
#define ICON_PAUSE           "\u04a2"
#define ICON_PLAY            "\u04a3"
#define ICON_FORWARD         "\u04a4"
#define ICON_BACKWARD        "\u04a5"

#define ICON_INW_ARROWS      "\u04c0"
#define ICON_7STAR           "\u04c1"
#define ICON_CROSSED_SWORDS  "\u04c2"
#define ICON_CIRCLE          "\u04c3"

#define ICON_WATER_EXTRACTOR "\u0408"
#define ICON_METAL_EXTRACTOR "\u0409"
#define ICON_HEATSHIELD      "\u0428"
#define ICON_FUELTANK        "\u0429"
#define ICON_SHIPYARD_1      "\u0448"
#define ICON_SHIPYARD_2      "\u0449"
#define ICON_SHIPYARD_3      "\u044A"
#define ICON_SHIPYARD_4      "\u044B"
#define ICON_RAILGUN         "\u0468"
#define ICON_MISSILE         "\u0469"
#define ICON_LIGHTARMOR      "\u046A"
#define ICON_ARMOR           "\u046B"
#define ICON_PDC             "\u046C"
#define ICON_ACS             "\u046E"

#define ICON_X "X "
#define ICON_LINK ICON_EMPTY

const double G = 6.6743015e-11;  // m³/(s²kg)

#endif  // CONSTANTS_H