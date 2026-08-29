/*
 * Receiver LoRa — Recovery WebUI
 * Recebe pacotes de telemetria via LoRa (915 MHz), adiciona hora/data do GPS local
 * e retransmite via Serial + salva no LittleFS.
 *
 * LittleFS: Para usar na IDE Arduino, selecione:
 *   Tools > Partition Scheme > "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
 */

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include "LittleFS.h"

// ── Config ──────────────────────────────────────
#define LORA_SCK        4
#define LORA_MOSI       3
#define LORA_MISO       2
#define LORA_CS         5
#define LORA_RST        6
#define LORA_IRQ        7
#define LORA_FREQUENCY  915E6
#define LORA_SYNC_WORD  0xF3
#define LORA_SF         7
#define LORA_BW         125E3
#define LORA_CR         5
#define LORA_TX_POWER   17
#define GPS_RX_PIN      20
#define GPS_TX_PIN      21
#define GPS_BAUD        9600
#define SERIAL_BAUD     115200

// ── GPS ─────────────────────────────────────────
struct GpsData {
    double   lat, lon, altMeters, speedKmph;
    uint32_t satellites;
    bool     valid;
};

struct GpsTimeData {
    uint32_t hhmmss, ddmmyyyy;
    bool     valid;
};

static TinyGPSPlus    _gps;
static HardwareSerial _gpsSerial(1);

static void gpsInit() {
    _gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

static void gpsProcess() {
    while (_gpsSerial.available() > 0) _gps.encode(_gpsSerial.read());
}

static GpsData gpsGetData() {
    GpsData d;
    d.valid      = _gps.location.isValid();
    d.lat        = d.valid ? _gps.location.lat()      : 0.0;
    d.lon        = d.valid ? _gps.location.lng()      : 0.0;
    d.altMeters  = _gps.altitude.isValid()  ? _gps.altitude.meters()  : 0.0;
    d.speedKmph  = _gps.speed.isValid()     ? _gps.speed.kmph()       : 0.0;
    d.satellites = _gps.satellites.isValid()? _gps.satellites.value() : 0;
    return d;
}

static GpsTimeData gpsGetTimeData() {
    GpsTimeData t;
    t.valid = _gps.time.isValid() && _gps.date.isValid();
    if (t.valid) {
        t.hhmmss   = _gps.time.hour() * 10000UL + _gps.time.minute() * 100UL + _gps.time.second();
        t.ddmmyyyy = _gps.date.day() * 1000000UL + _gps.date.month() * 10000UL + _gps.date.year();
    } else {
        t.hhmmss = t.ddmmyyyy = 0;
    }
    return t;
}

// ── LoRa Receiver ──────────────────────────────
static bool _loraReady = false;

static bool loraInit() {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
    if (!LoRa.begin(LORA_FREQUENCY)) { Serial.println("[LoRa] Falha"); return false; }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.enableCrc();
    LoRa.receive();
    _loraReady = true;
    return true;
}

static bool loraAvailable() {
    return _loraReady && LoRa.parsePacket() > 0;
}

static String loraReceive() {
    if (!_loraReady) return "";
    int sz = LoRa.parsePacket();
    if (sz <= 0) return "";
    String p;
    p.reserve(sz);
    while (LoRa.available()) p += (char)LoRa.read();
    LoRa.receive();
    return p;
}

static int loraLastRSSI() {
    return LoRa.packetRssi();
}

// ── Payload ─────────────────────────────────────
static String buildProtocolPacket(
    const String &team_id, uint32_t millis_ts, uint32_t count,
    float ax, float ay, float az, float gx, float gy, float gz,
    float temp, float press, float hum, float altp,
    float vz, float max_alt, int state,
    float lat, float lon, float alt, uint8_t sats,
    int parachute,
    uint32_t hora, uint32_t data_gps, int rssi
) {
    char buf[256];
    snprintf(buf, sizeof(buf),
        "%s,%lu,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%lu,%lu,%.2f,%.6f,%.6f,%u,%d,%d",
        team_id.c_str(), millis_ts, count,
        altp, temp, hum, press, gx, gy, gz, ax, ay, az,
        vz, max_alt, state,
        hora, data_gps, alt, lat, lon, sats, parachute, rssi);
    return String(buf);
}

// ── LittleFS ────────────────────────────────────
static String logPath;
static bool   fsReady = false;

static bool setupLittleFS() {
    if (!LittleFS.begin(true)) { Serial.println("[FS] Falha"); return false; }
    Serial.printf("[FS] LittleFS OK — %u bytes total\n", (unsigned)LittleFS.totalBytes());
    return true;
}

static String generateLogPath() {
    for (int i = 1; i <= 999; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/recovery_%03d.csv", i);
        if (!LittleFS.exists(path)) return String(path);
    }
    return String("/overflow.csv");
}

static void writeHeader(const char* path) {
    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) { Serial.println("[FS] Erro ao criar cabecalho"); return; }
    f.println("millis,TEAM_ID,millis_ts,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi");
    f.close();
}

static void appendLog(const String &line) {
    if (!fsReady) return;
    File f = LittleFS.open(logPath.c_str(), FILE_APPEND);
    if (!f) { Serial.println("[FS] Erro ao abrir para append"); return; }
    f.println(line);
    f.close();
}

// ── Main ───────────────────────────────────────
static uint32_t lastCount = 0, totalReceived = 0, totalLost = 0, totalErrors = 0;
static bool     firstPacket = true;

static bool parseSatellitePacket(const String &raw,
    String &team_id, uint32_t &millis_ts, uint32_t &count,
    float &ax, float &ay, float &az, float &gx, float &gy, float &gz,
    float &temp, float &press, float &hum, float &altp,
    float &vz, float &max_alt, int &state,
    float &lat, float &lon, float &alt, uint8_t &sats,
    int &parachute, int &rssi_placeholder
) {
    int commas = 0;
    for (unsigned i = 0; i < raw.length(); i++) if (raw[i] == ',') commas++;
    if (commas < 21) { Serial.println("[PARSE] Incompleto: " + String(commas + 1) + " campos"); return false; }

    int start = 0;
    auto next = [&](String &out) {
        int n = raw.indexOf(',', start);
        if (n == -1) { out = raw.substring(start); start = raw.length(); }
        else         { out = raw.substring(start, n); start = n + 1; }
    };

    String f;
    next(team_id); next(f); millis_ts = f.toInt();
    next(f); count = f.toInt(); next(f); altp = f.toFloat();
    next(f); temp = f.toFloat(); next(f); hum = f.toFloat();
    next(f); press = f.toFloat(); next(f); gx = f.toFloat();
    next(f); gy = f.toFloat(); next(f); gz = f.toFloat();
    next(f); ax = f.toFloat(); next(f); ay = f.toFloat();
    next(f); az = f.toFloat(); next(f); vz = f.toFloat();
    next(f); max_alt = f.toFloat(); next(f); state = f.toInt();
    next(f); alt = f.toFloat(); next(f); lat = f.toFloat();
    next(f); lon = f.toFloat(); next(f); sats = f.toInt();
    next(f); parachute = f.toInt(); next(f); rssi_placeholder = f.toInt();
    return true;
}

static void trackPacketCount(uint32_t count) {
    totalReceived++;
    if (firstPacket) { lastCount = count; firstPacket = false; return; }
    if (count == lastCount + 1) return;
    if (count > lastCount + 1) {
        uint32_t lost = count - lastCount - 1;
        totalLost += lost;
        Serial.printf("[LOST] %u pacote(s) perdido(s) %u->%u | total: %u\n", lost, lastCount, count, totalLost);
    } else {
        Serial.printf("[STATS] Count resetado %u->%u (possivel reboot)\n", lastCount, count);
    }
    lastCount = count;
}

void setup() {
    Serial.begin(SERIAL_BAUD); delay(400);
    Serial.println("\n=== Recovery System — LoRa Receiver ===\n");
    gpsInit();
    _loraReady = loraInit();

    fsReady = setupLittleFS();
    if (fsReady) {
        logPath = generateLogPath();
        writeHeader(logPath.c_str());
        Serial.printf("[FS] Log gravando em: %s\n", logPath.c_str());
    }

    Serial.println("\n[SYS] Aguardando pacotes LoRa...\n");
}

void loop() {
    gpsProcess();
    if (!_loraReady || !loraAvailable()) { delay(10); return; }

    String raw = loraReceive();
    if (raw.length() == 0) return;

    int rxRssi = loraLastRSSI();
    Serial.print("[LoRa] RX (RSSI=" + String(rxRssi) + "): ");
    Serial.println(raw);

    String team_id;
    uint32_t millis_ts, count;
    float ax, ay, az, gx, gy, gz, temp, press, hum, altp, vz, max_alt;
    int state;
    float lat, lon, alt;
    uint8_t sats;
    int parachute;
    int rssi_placeholder;

    if (!parseSatellitePacket(raw, team_id, millis_ts, count,
        ax, ay, az, gx, gy, gz, temp, press, hum, altp,
        vz, max_alt, state,
        lat, lon, alt, sats, parachute, rssi_placeholder)) {
        totalErrors++;
        return;
    }

    trackPacketCount(count);
    GpsTimeData t = gpsGetTimeData();
    String pkt = buildProtocolPacket(team_id, millis_ts, count,
        ax, ay, az, gx, gy, gz, temp, press, hum, altp,
        vz, max_alt, state,
        lat, lon, alt, sats, parachute,
        t.hhmmss, t.ddmmyyyy, rxRssi);
    Serial.println(pkt);

    String logLine = String(millis()) + "," + pkt;
    appendLog(logLine);
}
