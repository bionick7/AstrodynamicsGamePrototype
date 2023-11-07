//const double G = 6.6743015e-11;  // m³/(s²kg)

const double PARENT_MU = 3.793e+16;  // m³/s²
const double PARENT_RADIUS = 58.232e+6;  // m

const char* PLANET_NAMES[] = {
    "Mimas",
    "Encelladus",
    "Thetys",
    "Rhea",
    "Titan",
    "Iaeptus",
    "Phoebe"
};

const char* SHIP_NAMES[] = {
    "Bulk transport",
    "Light transport",
    "Express"
};

const unsigned long PLANET_PROGRADE_FLAGS = 0b0111111;

// SMA, Ecc, Ann, LoP, radius, mu
double PLANET_TABLE[] = {
      198.200e+6, 0.020, 275.0 * DEG2RAD, ( 40.6 + 160.0) * DEG2RAD,  198.0e+3,  1.8022e+21 * G,  // Mimas
      237.905e+6, 0.005,  57.0 * DEG2RAD, ( 40.6 + 119.5) * DEG2RAD,  252.0e+3,  1.8022e+21 * G,  // Encelladus
      294.619e+6, 0.001,   0.0 * DEG2RAD, ( 40.6 + 335.3) * DEG2RAD,  533.0e+3, 6.17449e+21 * G,  // Tethys
      527.108e+6, 0.001,  31.5 * DEG2RAD, ( 40.6 +  44.3) * DEG2RAD,  764.0e+3, 6.17449e+21 * G,  // Rhea
     1221.930e+6, 0.029,  11.7 * DEG2RAD, ( 36.4 +  78.3) * DEG2RAD, 2574.0e+3,  1345.2e+21 * G,  // Titan
     3560.820e+6, 0.028,  74.8 * DEG2RAD, (288.7 + 254.5) * DEG2RAD,  252.0e+3,  14.686e+21 * G,  // Iaeptus
    12929.400e+6, 0.164, 308.0 * DEG2RAD, (276.0 + 240.3) * DEG2RAD,  106.0e+3,  8.3129e+18 * G,  // Phoebe
};

// WATER FOOD in t/day
double PLANET_RESOURCE_DELTA_TABLE[] = {
    100,     0,  // Mimas
    400,  -100,  // Encelladus
    200,   300,  // Tethys
      0,     0,  // Rhea
    200,  -100,  // Titan
      0,     0,  // Iaeptus
      0,     0   // Phoebe
};

// WATER FOOD in t
double PLANET_RESOURCE_STOCK_TABLE[] = {
     0,     0,  // Mimas
  1000,  2000,  // Encelladus
  1000,  2000,  // Tethys
     0,     0,  // Rhea
  5000,  2000,  // Titan
     0,     0,  // Iaeptus
     0,     0   // Phoebe
};

// cargo capacity(t), dv_max(km/s), v_ex(km/s)
double SHIP_STAT_TABLE[] = {
    3000, 10, 10,   // Bulk transport
    1000, 10, 10,   // Fast mover
    1000, 30, 30,   // Express
};
