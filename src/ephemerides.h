const double G = 6.6743015e-11;  // m³/(s²kg)
const double deg2rad = 180 / 3.141592;

const double mu_parent = 3.793e+16;  // m³/s²
const double radius_parent = 58.232e+6;  // m

const char* planet_names[] = {
    "Mimas",
    "Encelladus",
    "Thetys",
    "Rhea",
    "Titan",
    "Iaeptus",
    "Phoebe"
};

const unsigned long planet_orbit_is_prograde = 0b0111111;

// SMA, Ecc, Ann, LoP, radius, mu
double planet_params[] = {
      198.200e+6, 0.020, 275.0 * deg2rad, ( 40.6 + 160.0) * deg2rad,  198.0e+3,  1.8022e+21 * G,  // Mimas
      237.905e+6, 0.005,  57.0 * deg2rad, ( 40.6 + 119.5) * deg2rad,  252.0e+3,  1.8022e+21 * G,  //  Encelladus
      294.619e+6, 0.001,   0.0 * deg2rad, ( 40.6 + 335.3) * deg2rad,  533.0e+3, 6.17449e+21 * G,  //  Tethys
      527.108e+6, 0.001,  31.5 * deg2rad, ( 40.6 +  44.3) * deg2rad,  764.0e+3, 6.17449e+21 * G,  //  Rhea
     1221.930e+6, 0.029,  11.7 * deg2rad, ( 36.4 +  78.3) * deg2rad, 2574.0e+3,  1345.2e+21 * G,  //  Titan
     3560.820e+6, 0.028,  74.8 * deg2rad, (288.7 + 254.5) * deg2rad,  252.0e+3,  14.686e+21 * G,  //  Iaeptus
    12929.400e+6, 0.164, 308.0 * deg2rad, (276.0 + 240.3) * deg2rad,  106.0e+3,  8.3129e+18 * G,  //  Phoebe
};
