#include <Arduino.h>
#include <TinyGPS++.h>
#include "GpsModule.h"
#include "config.h"

static TinyGPSPlus  _gps;
static HardwareSerial _serial(1);   // UART1
static bool _hadFix = false;

void gpsInit() {
    _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] UART1 iniciada");
}

void gpsProcess() {
    while (_serial.available() > 0) {
        _gps.encode(_serial.read());
    }
}

bool gpsHasFix() {
    return _gps.location.isValid();
}

GpsData gpsGetData() {
    GpsData d;
    d.valid      = _gps.location.isValid();
    d.lat        = d.valid ? _gps.location.lat()      : 0.0;
    d.lon        = d.valid ? _gps.location.lng()      : 0.0;
    d.altMeters  = _gps.altitude.isValid()  ? _gps.altitude.meters()  : 0.0;
    d.speedKmph  = _gps.speed.isValid()     ? _gps.speed.kmph()       : 0.0;
    d.satellites = _gps.satellites.isValid()? _gps.satellites.value() : 0;
    return d;
}

GpsTimeData gpsGetTimeData() {
    GpsTimeData t;
    t.valid = _gps.time.isValid() && _gps.date.isValid();

    if (t.valid) {
        // HH:MM:SS -> HHMMSS
        t.hhmmss = (uint32_t)(_gps.time.hour() * 10000UL +
                              _gps.time.minute() * 100UL +
                              _gps.time.second());
        // DD/MM/YYYY -> DDMMYYYY
        t.ddmmyyyy = (uint32_t)(_gps.date.day() * 1000000UL +
                                _gps.date.month() * 10000UL +
                                _gps.date.year());
    } else {
        t.hhmmss = 0;
        t.ddmmyyyy = 0;
    }
    return t;
}
