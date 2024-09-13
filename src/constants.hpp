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

    void ExportToFile(const char* filename);
}

#define ICON_EMPTY "\u04ff"  // lower-right corner
#define ICON_PLANET "\u0300"

#define ICON_QUESTMANAGER   "\u0301"
#define ICON_TIMELINE       "\u0302"
#define ICON_BATTLELOG      "\u0303"
#define ICON_RESEARCHSCREEN "\u0304"

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

#define ICON_MODULE_TYPE_LARGE  "\u03E0"
#define ICON_MODULE_TYPE_MEDIUM "\u03E1"
#define ICON_MODULE_TYPE_SMALL  "\u03E2"
#define ICON_MODULE_TYPE_FREE   "\u03E3"
#define ICON_MODULE_TYPE_ARMOR  "\u03E4"
#define ICON_MODULE_TYPE_DROP   "\u03E5"
#define ICON_MODULE_TYPE_ANY    "\u03E6"

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

#define ICON_WATER_EXTRACTOR          "\u0408"
#define ICON_METAL_EXTRACTOR          "\u0409"
#define ICON_HEATSHIELD               "\u0428"
#define ICON_FUELTANK                 "\u0429"
#define ICON_INDUSTRIAL_ADMIN         "\u0448"
#define ICON_INDUSTRIAL_STORAGE       "\u0449"
#define ICON_INDUSTRIAL_MANUFACTURING "\u044A"
#define ICON_INDUSTRIAL_DOCK          "\u044B"
#define ICON_RAILGUN                  "\u0468"
#define ICON_MISSILE                  "\u0469"
#define ICON_LIGHTARMOR               "\u046A"
#define ICON_ARMOR                    "\u046B"
#define ICON_PDC                      "\u046C"
#define ICON_ACS                      "\u046E"

#define ICON_CLOCK_EMPTY          "\u04e0"
#define ICON_CLOCK_BARELY         "\u04e1"
#define ICON_CLOCK_1_8            "\u04e2"
#define ICON_CLOCK_QUARTER        "\u04e3"
#define ICON_CLOCK_3_8            "\u04e4"
#define ICON_CLOCK_HALF           "\u04e5"
#define ICON_CLOCK_5_8            "\u04e6"
#define ICON_CLOCK_THREEQUARTERS  "\u04e7"
#define ICON_CLOCK_7_8            "\u04e8"
#define ICON_CLOCK_ALMOST         "\u04e9"
#define ICON_CLOCK_FULL           "\u04ea"

// Aliases for ship stats

#define ICON_INITIATIVE       ICON_ACS
#define ICON_KINETIC_HP       ICON_HEART_KINETIC
#define ICON_ENERGY_HP        ICON_HEART_ENERGY
#define ICON_CREW             ICON_HEART_BOARDING
#define ICON_KINETIC_OFFENSE  ICON_ATTACK_KINETIC
#define ICON_ORDNANCE_OFFENSE ICON_ATTACK_ORDNANCE
#define ICON_BOARDING_OFFENSE ICON_ATTACK_BOARDING
#define ICON_KINETIC_DEFENSE  ICON_SHIELD_KINETIC
#define ICON_ORDNANCE_DEFENSE ICON_SHIELD_ORDNANCE
#define ICON_BOARDING_DEFENSE ICON_SHIELD_BOARDING

#define ICON_GROUND_CONNECTION        "\u0340"
#define ICON_THERMAL_CONTROL          "\u0341"
#define ICON_CRYOGENICS_FACILITY      "\u0342"
#define ICON_CLEAN_ROOM               "\u0343"
#define ICON_BIO_MANUFACTURING        "\u0344"
#define ICON_ARMS_MANUFACTURING       "\u0345"
#define ICON_PRECISION_MANUFACTURING  "\u0346"
#define ICON_NUCLEAR_ENRICHMENT       "\u0347"
#define ICON_MILITARY_TRAINING        "\u0348"

#define ICON_X "X "
#define ICON_LINK ICON_EMPTY

const double G = 6.6743015e-11;  // m³/(s²kg)

#endif  // CONSTANTS_H