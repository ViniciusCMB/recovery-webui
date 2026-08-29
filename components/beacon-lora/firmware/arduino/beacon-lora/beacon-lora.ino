#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>

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
#define TX_INTERVAL_MS  200
#define TEAM_ID         "#213"

// ── GPS ─────────────────────────────────────────
struct GpsData {
    double   lat, lon, altMeters, speedKmph;
    uint32_t satellites;
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

// ── LoRa Transmitter ────────────────────────────
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
    _loraReady = true;
    return true;
}

static bool loraSend(const String &payload) {
    if (!_loraReady) return false;
    LoRa.beginPacket();
    LoRa.print(payload);
    return LoRa.endPacket() == 1;
}

// ── Payload ─────────────────────────────────────
struct BeaconData {
    uint32_t millis, count;
    float altp, temp, umi, p;
    float gp, gr, gy, ap, ar, ay;
    float alt, lat, lon;
    uint8_t sat, pqd;
};

static String buildBeaconPacket(const String &team_id, const BeaconData &d) {
    char buf[200];
    snprintf(buf, sizeof(buf),
        "%s,%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%u,%u,%d",
        team_id.c_str(), d.millis, d.count,
        d.altp, d.temp, d.umi, d.p,
        d.gp, d.gr, d.gy, d.ap, d.ar, d.ay,
        d.alt, d.lat, d.lon, d.sat, d.pqd, -1);
    return String(buf);
}

// ── Main ───────────────────────────────────────
static uint32_t packetCount = 0;

void setup() {
    Serial.begin(SERIAL_BAUD); delay(400);
    Serial.println("\n=== Beacon LoRa — Simulador ===\n");
    gpsInit();
    if (!loraInit()) { Serial.println("[SYS] Falha LoRa"); while (1) delay(1000); }
    Serial.println("[SYS] Beacon pronto — heartbeat a cada " + String(TX_INTERVAL_MS) + "ms\n");
}

void loop() {
    gpsProcess();
    static uint32_t lastTx = 0;
    uint32_t now = millis();
    if (now - lastTx < TX_INTERVAL_MS) return;
    lastTx = now;

    GpsData gps = gpsGetData();
    BeaconData d = {0};
    d.millis = now;
    d.count  = packetCount++;
    d.alt = gps.altMeters;
    d.lat = gps.lat;
    d.lon = gps.lon;
    d.sat = (uint8_t)gps.satellites;

    String pkt = buildBeaconPacket(TEAM_ID, d);
    loraSend(pkt);

    Serial.print("[SYS] Heartbeat #");
    Serial.print(d.count);
    if (gps.valid) { Serial.printf(" | GPS: %.6f, %.6f (%u sat)", gps.lat, gps.lon, gps.satellites); }
    else { Serial.print(" | GPS: sem fix"); }
    Serial.println();
}
