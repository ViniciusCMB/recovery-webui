#pragma once
#include <TinyGPS++.h>

struct GpsData {
    double   lat;
    double   lon;
    double   altMeters;
    double   speedKmph;
    uint32_t satellites;
    bool     valid;
};

/**
 * @brief Hora GPS em formato HHMMSS (uint32).
 * Retorna 0 se GPS invalido ou sem fix.
 */
struct GpsTimeData {
    uint32_t hhmmss;    // ex: 143045
    uint32_t ddmmyyyy;  // ex: 20012025
    bool     valid;
};

void        gpsInit();
void        gpsProcess();      // Chamar a cada loop()
bool        gpsHasFix();
GpsData     gpsGetData();
GpsTimeData gpsGetTimeData();  // Hora/data formatadas
