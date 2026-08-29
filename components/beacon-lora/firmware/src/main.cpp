#include <Arduino.h>
#include "config.h"
#include "LoraTransmitter.h"
#include "GpsModule.h"
#include "payload.h"

// ── Estado do satellite ouvido ──────────────────
static bool     heardSat   = false;   // true apos o primeiro pacote do satellite
static uint32_t lastSatCount  = 0;    // count do ultimo pacote do satellite ouvido
static uint32_t lastSatMillis = 0;    // millis (beacon) em que ouviu esse pacote
static int      lastSatRssi   = -127; // RSSI (dBm) desse pacote

static uint32_t packetCount   = 0;    // contador local de TX (debug/perda)
static uint32_t lastTxMillis  = 0;    // millis do ultimo TX efetivamente enviado

// ── TX agendado (com jitter anti-colisao) ───────
static bool     pendingTx    = false;
static uint32_t pendingTxAt  = 0;     // millis em que o TX deve ocorrer

/**
 * @brief Faz parse do pacote do satellite (formato real Helike, 18 campos).
 *
 * Igual ao receiver: aceita marcador '#' final (o satellite real envia) e so
 * interessa quando TEAM_ID == SAT_TEAM_ID. O count fica na posicao 2.
 *
 * @return true se e' pacote do satellite; preenche count.
 */
static bool parseSatellitePacket(const String &raw, uint32_t &count) {
    String line = raw;
    if (line.endsWith("#")) {
        line = line.substring(0, line.length() - 1);
    }

    // Satellite #213: 18 campos = 17 virgulas (Helike transmite sem
    // vz/maxAltitude/state/parachute; foguetes #11/#51 usam 22 campos e
    // tambem sao aceitos — basta TEAM_ID e count na posicao 2)
    int commaCount = 0;
    for (unsigned i = 0; i < line.length(); i++) {
        if (line[i] == ',') commaCount++;
    }
    if (commaCount < 17) return false;

    int start = 0;
    auto nextField = [&](String &out) {
        int next = line.indexOf(',', start);
        if (next == -1) {
            out = line.substring(start);
            start = line.length();
        } else {
            out = line.substring(start, next);
            start = next + 1;
        }
    };

    String field, team;
    nextField(team);            // 0 TEAM_ID
    if (team != SAT_TEAM_ID) return false;
    nextField(field);           // 1 millis (nao usado pelo beacon)
    nextField(field); count = field.toInt();  // 2 count

    return true;
}

/**
 * @brief Agenda o proximo TX do report com jitter aleatorio.
 *
 * Se ja ha um TX pendente, apenas atualiza os dados (o pacote mais recente
 * ouvido vence) e o TX acontece no horario ja agendado.
 *
 * O horario respeita REPORT_MIN_INTERVAL_MS desde o ultimo TX real, para
 * limitar o duty cycle do canal — e o jitter espalha os beacons no tempo.
 */
static void scheduleTx() {
    uint32_t now = millis();
    if (!pendingTx) {
        pendingTx = true;
        uint32_t due = now + random(TX_JITTER_MIN_MS, TX_JITTER_MAX_MS + 1);
        if (due < lastTxMillis + REPORT_MIN_INTERVAL_MS) {
            due = lastTxMillis + REPORT_MIN_INTERVAL_MS;
        }
        pendingTxAt = due;
    }
}

void setup() {
    // Entropia para o jitter anti-colisao dos reports
    randomSeed((uint32_t)micros() ^ (uint32_t)(uintptr_t)&pendingTx);

    Serial.begin(SERIAL_BAUD);
    delay(400);
    Serial.println();
    Serial.println("=== Beacon LoRa — Escuta + Report (Mesh) ===");
    Serial.print("BEACON_ID: ");
    Serial.println(BEACON_ID);
    Serial.print("Escutando o satellite: ");
    Serial.println(SAT_TEAM_ID);
    Serial.println();

    gpsInit();

    if (!loraInit()) {
        Serial.println("[SYS] Falha critica no LoRa — travando");
        while (1) delay(1000);
    }

    Serial.println();
    Serial.println("[SYS] Beacon pronto — report a cada pacote do satellite "
                   "(min " + String(REPORT_MIN_INTERVAL_MS) + "ms) + heartbeat de "
                   + String(HEARTBEAT_INTERVAL_MS) + "ms");
    Serial.println();
}

void loop() {
    gpsProcess();
    uint32_t now = millis();

    // ── 1) Escuta o satellite ───────────────────
    if (loraAvailable()) {
        String raw = loraReceive();
        int rxRssi = loraLastRSSI();
        uint32_t satCount = 0;

        if (parseSatellitePacket(raw, satCount)) {
            heardSat = true;
            lastSatCount  = satCount;
            lastSatMillis = now;
            lastSatRssi   = rxRssi;
            scheduleTx();
            Serial.println("[LoRa] SAT pkt#" + String(satCount)
                           + " (RSSI=" + String(rxRssi) + " dBm)");
        }
        // Pacotes de outros dispositivos (#Bx, #11, #51...) sao ignorados.
    }

    // ── 2) Heartbeat quando nao ouve o satellite ─
    if (!heardSat || (now - lastSatMillis >= HEARTBEAT_INTERVAL_MS)) {
        if (!pendingTx && (now - lastTxMillis >= HEARTBEAT_INTERVAL_MS)) {
            pendingTx = true;
            pendingTxAt = now + random(0, TX_JITTER_MAX_MS + 1);
        }
    }

    // ── 3) Envio do report agendado ─────────────
    if (!pendingTx || now < pendingTxAt) return;
    pendingTx = false;
    lastTxMillis = now;

    GpsData gps = gpsGetData();
    bool recentlyHeard = heardSat && (now - lastSatMillis < HEARTBEAT_INTERVAL_MS);

    BeaconReport r = {0};
    r.millis      = now;
    r.count       = packetCount++;
    r.sat_count   = recentlyHeard ? lastSatCount : 0;
    r.sat_millis  = recentlyHeard ? lastSatMillis : 0;
    r.lat         = gps.valid ? (float)gps.lat : 0.0f;
    r.lon         = gps.valid ? (float)gps.lon : 0.0f;
    r.alt         = gps.valid ? (float)gps.altMeters : 0.0f;
    r.sat         = (uint8_t)gps.satellites;
    r.rssi        = recentlyHeard ? lastSatRssi : -127;
    r.gpsv        = gps.valid ? 1 : 0;

    String packet = buildBeaconReport(BEACON_ID, r);
    loraSend(packet);

    Serial.print("[SYS] Report #");
    Serial.print(r.count);
    Serial.print(" | sat_pkt=");
    Serial.print(r.sat_count);
    Serial.print(" | RSSI=");
    Serial.print(r.rssi);
    Serial.print(" | GPS: ");
    if (gps.valid) {
        Serial.print(gps.lat, 6);
        Serial.print(", ");
        Serial.print(gps.lon, 6);
        Serial.print(" (");
        Serial.print(gps.satellites);
        Serial.print(" sat)");
    } else {
        Serial.print("sem fix");
    }
    Serial.println();
}