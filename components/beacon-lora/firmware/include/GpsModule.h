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

void    gpsInit();
void    gpsProcess();      // Chamar a cada loop()
bool    gpsHasFix();
GpsData gpsGetData();
