#include <Arduino.h>
#include "LittleFS.h"
#include "config.h"
#include "GpsModule.h"
#include "LoraReceiver.h"
#include "payload.h"
#include "Trilateration.h"

static bool loraReady = false;

// ── Contagem de pacotes perdidos e estatisticas ──
static uint32_t lastCount       = 0;    // ultimo count recebido do satellite
static uint32_t totalReceived   = 0;    // total de pacotes recebidos com sucesso
static uint32_t totalLost       = 0;    // total de pacotes perdidos (saltos no count)
static uint32_t totalErrors     = 0;    // total de pacotes com parse falho
static bool     firstPacket     = true; // true ate receber o primeiro pacote

// ── LittleFS ────────────────────────────────────
static String logPath;
static bool   fsReady = false;

// ══════════════════════════════════════════════════════════════════════
// Mesh de beacons — trilateracao GPS do satellite
// ══════════════════════════════════════════════════════════════════════

/**
 * Report do beacon recebido pelo receiver (formato: ver payload.h do beacon).
 * #B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
 */
struct BeaconReportRx {
    uint32_t millis;      // relogio do beacon no TX
    uint32_t count;       // contador local de TX do beacon
    uint32_t sat_count;   // count do ultimo pacote do satellite ouvido (0 = heartbeat)
    uint32_t sat_millis;  // relogio do beacon quando ouviu esse pacote
    float    lat, lon, alt;
    uint8_t  sat;
    int      rssi;
    uint8_t  gpsv;
};

/** Estado de cada beacon (para o check "ativos" e "ouvindo"). */
struct BeaconStatus {
    char     id[8];            // "B1".."B8"
    uint32_t lastSeen;         // millis do receiver da ultima report
    uint32_t lastMillis;       // relogio do beacon na ultima report
    uint32_t lastSatCount;     // sat_count da ultima report
    uint32_t lastSatMillis;    // sat_millis da ultima report
    int      rssi;             // RSSI reportado
    bool     gpsValid;
    uint8_t  gpsSats;
};
static BeaconStatus beacons[TRILAT_MAX_BEACONS];
static uint8_t      beaconCount = 0;

/**
 * Cesto de sincronizacao: um slot por pacote #213 recebido, aguardando os
 * reports dos beacons que ouviram o MESMO pacote (mesmo `count`). So com
 * >= TRILAT_MIN_LISTENERS ouvintes o slot e' emitido com posicao corrigida.
 */
struct PendingPacket {
    bool     inUse;
    bool     emitted;
    bool     corrected;
    uint32_t count;            // count do pacote do satellite
    uint32_t rxMillis;         // millis do receiver quando ouviu o pacote
    String   raw;              // pacote #213 original (sem '#')
    int      rxRssi;           // RSSI medido pelo proprio receiver
    float    satAltp;          // altitude barometrica do satellite (correcao de altitude)
    ListenerSample samples[TRILAT_MAX_BEACONS + 1];
    uint8_t  nSamples;
};
static PendingPacket basket[TRILAT_BASKET_SLOTS];

// Forward: definida na secao LittleFS (usada por emitProtocolPacket)
static void appendLog(const String &line);

// ── Helpers do mesh ─────────────────────────────

static BeaconStatus* findBeacon(const char* id) {
    for (uint8_t i = 0; i < beaconCount; i++) {
        if (strcmp(beacons[i].id, id) == 0) return &beacons[i];
    }
    return nullptr;
}

static void upsertBeacon(const char* id, const BeaconReportRx &r) {
    BeaconStatus *b = findBeacon(id);
    if (b == nullptr) {
        if (beaconCount >= TRILAT_MAX_BEACONS) return;
        b = &beacons[beaconCount++];
        strncpy(b->id, id, sizeof(b->id) - 1);
        b->id[sizeof(b->id) - 1] = '\0';
        Serial.println("[MESH] Novo beacon detectado: " + String(id));
    }
    b->lastSeen      = millis();
    b->lastMillis    = r.millis;
    b->lastSatCount  = r.sat_count;
    b->lastSatMillis = r.sat_millis;
    b->rssi          = r.rssi;
    b->gpsValid      = r.gpsv != 0;
    b->gpsSats       = r.sat;
}

/** Adiciona/atualiza a amostra de um ouvinte no cesto (mais recente vence). */
static void addSample(PendingPacket &p, const char* id,
                      double lat, double lon, float alt, float rssi) {
    for (uint8_t i = 0; i < p.nSamples; i++) {
        if (strcmp(p.samples[i].id, id) == 0) {
            p.samples[i].lat    = lat;
            p.samples[i].lon    = lon;
            p.samples[i].altM   = alt;
            p.samples[i].rssi   = rssi;
            p.samples[i].rangeM = rssiToGroundRange(
                rssi, TRILAT_TX_POWER_DBM, TRILAT_PATH_LOSS_N,
                p.satAltp - alt);
            return;
        }
    }
    if (p.nSamples >= TRILAT_MAX_BEACONS + 1) return;
    ListenerSample &s = p.samples[p.nSamples++];
    strncpy(s.id, id, sizeof(s.id) - 1);
    s.id[sizeof(s.id) - 1] = '\0';
    s.lat    = lat;
    s.lon    = lon;
    s.altM   = alt;
    s.rssi   = rssi;
    s.rangeM = rssiToGroundRange(rssi, TRILAT_TX_POWER_DBM, TRILAT_PATH_LOSS_N,
                                 p.satAltp - alt);
}

/**
 * @brief Faz parse do CSV recebido via LoRa (dois formatos, por TEAM_ID).
 *
 * Foguete #11/#51 (flight-computer v2.0, 22 campos):
 *   TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,
 *   maxAltitude,state,alt,lat,lon,sat,parachute,rssi
 *
 * Satellite #213 (Helike, 18 campos — sem vz/maxAltitude/state/parachute):
 *   TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,alt,lat,lon,sat,rssi
 *   gp/gr/gy = giroscopio, ap/ar/ay = acelerometro (rad/s e m/s2).
 *   Os campos ausentes sao zerados na saida (mantem o emissor v2.0 de 24 campos).
 *
 * O marcador '#' final e' ENVIADO pelo satellite real (fim de pacote RF) e
 * aceito como opcional nos demais. Pacotes truncados/corrompidos (menos
 * campos que o formato do TEAM_ID) sao descartados.
 *
 * Retorna true se o parse foi bem sucedido.
 */
static bool parseSatellitePacket(
    const String &raw,
    String &team_id,
    uint32_t &millis_ts,
    uint32_t &count,
    float &ax, float &ay, float &az,
    float &gx, float &gy, float &gz,
    float &temp, float &press, float &hum,
    float &altp,
    float &vz,
    float &max_alt,
    int   &state,
    float &lat, float &lon, float &alt,
    uint8_t &sats,
    int   &parachute,
    int   &rssi_placeholder
) {
    // Marcador '#' final (protecao contra truncamento)
    String line = raw;
    if (line.endsWith("#")) {
        line = line.substring(0, line.length() - 1);
    }

    // Conta campos para validar contra o formato do TEAM_ID
    int commaCount = 0;
    for (unsigned i = 0; i < line.length(); i++) {
        if (line[i] == ',') commaCount++;
    }

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

    String field;
    nextField(team_id);                // 0  TEAM_ID

    bool isSat = (team_id == SAT_TEAM_ID);
    int required = isSat ? 17 : 21;    // #213: 18 campos; outros: 22 campos
    if (commaCount < required) {
        Serial.println("[PARSE] Pacote incompleto: " + String(commaCount + 1)
                       + " campos (esperado " + String(required + 1) + ")");
        return false;
    }

    nextField(field); millis_ts = field.toInt();   // 1  millis
    nextField(field); count = field.toInt();        // 2  count
    nextField(field); altp = field.toFloat();       // 3  altp
    nextField(field); temp = field.toFloat();       // 4  temp
    nextField(field); hum = field.toFloat();        // 5  umi (0 se sem sensor)
    nextField(field); press = field.toFloat();      // 6  p
    nextField(field); gx = field.toFloat();         // 7  gx (sat: gp)
    nextField(field); gy = field.toFloat();         // 8  gy (sat: gr)
    nextField(field); gz = field.toFloat();         // 9  gz (sat: gy)
    nextField(field); ax = field.toFloat();         // 10 ax (sat: ap)
    nextField(field); ay = field.toFloat();         // 11 ay (sat: ar)
    nextField(field); az = field.toFloat();         // 12 az (sat: ay)

    if (isSat) {
        // Satellite #213: 18 campos — alt GPS vem antes de lat/lon, sem
        // vz/maxAltitude/state/parachute (zerados para o emissor v2.0)
        nextField(field); alt = field.toFloat();        // 13 alt (GPS)
        nextField(field); lat = field.toFloat();        // 14 lat
        nextField(field); lon = field.toFloat();        // 15 lon
        nextField(field); sats = (uint8_t)field.toInt(); // 16 sat
        nextField(field); rssi_placeholder = field.toInt(); // 17 rssi (placeholder)
        vz = 0.0f;
        max_alt = 0.0f;
        state = 0;
        parachute = 0;
    } else {
        // Foguete #11/#51: formato v2.0 (22 campos)
        nextField(field); vz = field.toFloat();         // 13 vz
        nextField(field); max_alt = field.toFloat();    // 14 maxAltitude
        nextField(field); state = field.toInt();        // 15 state
        nextField(field); alt = field.toFloat();        // 16 alt (GPS)
        nextField(field); lat = field.toFloat();        // 17 lat
        nextField(field); lon = field.toFloat();        // 18 lon
        nextField(field); sats = (uint8_t)field.toInt(); // 19 sat
        nextField(field); parachute = field.toInt();    // 20 parachute (0/1)
        nextField(field); rssi_placeholder = field.toInt(); // 21 rssi (placeholder)
    }

    return true;
}

/**
 * @brief Faz parse do report do beacon (11 campos).
 *
 * #B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
 *
 * @return true se o parse foi bem sucedido; idOut recebe "B1".."B8".
 */
static bool parseBeaconReport(const String &raw, char *idOut, size_t idSize,
                              BeaconReportRx &r) {
    String line = raw;
    if (line.endsWith("#")) {
        line = line.substring(0, line.length() - 1);
    }

    // 11 campos = 10 virgulas
    int commaCount = 0;
    for (unsigned i = 0; i < line.length(); i++) {
        if (line[i] == ',') commaCount++;
    }
    if (commaCount < 10) return false;

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
    nextField(team);                     // 0 TEAM_ID (#B1..)
    if (!team.startsWith("#B")) return false;
    strncpy(idOut, team.c_str() + 1, idSize - 1);  // remove '#'
    idOut[idSize - 1] = '\0';

    nextField(field); r.millis     = field.toInt();       // 1 millis
    nextField(field); r.count      = field.toInt();       // 2 count
    nextField(field); r.sat_count  = field.toInt();       // 3 sat_count
    nextField(field); r.sat_millis = field.toInt();       // 4 sat_millis
    nextField(field); r.lat        = field.toFloat();     // 5 lat
    nextField(field); r.lon        = field.toFloat();     // 6 lon
    nextField(field); r.alt        = field.toFloat();     // 7 alt
    nextField(field); r.sat        = (uint8_t)field.toInt(); // 8 sat
    nextField(field); r.rssi       = field.toInt();       // 9 rssi
    nextField(field); r.gpsv       = (uint8_t)field.toInt(); // 10 gpsv

    return true;
}

/**
 * @brief Emite o pacote de protocolo (24 campos) via Serial + LittleFS.
 *
 * @param latOverride,lonOverride Ponteiros para a posicao corrigida por
 *        trilateracao; nullptr usa o GPS cru do pacote do satellite.
 */
static void emitProtocolPacket(PendingPacket &p,
                               const float *latOverride,
                               const float *lonOverride) {
    String team_id;
    uint32_t millis_ts, count;
    float ax, ay, az, gx, gy, gz;
    float temp, press, hum, altp;
    float vz, max_alt;
    int state;
    float rawLat, rawLon, alt;
    uint8_t sats;
    int parachute, rssi_placeholder;

    if (!parseSatellitePacket(p.raw, team_id, millis_ts, count,
                              ax, ay, az, gx, gy, gz,
                              temp, press, hum, altp,
                              vz, max_alt, state,
                              rawLat, rawLon, alt, sats,
                              parachute, rssi_placeholder)) {
        // Re-parse deveria sempre passar (o pacote ja foi validado no push)
        Serial.println("[FS] ERRO interno no re-parse do pacote #" + String(p.count));
        return;
    }

    // Posicao final: corrigida por trilateracao ou GPS cru do satellite
    float lat = latOverride ? *latOverride : rawLat;
    float lon = lonOverride ? *lonOverride : rawLon;

    // Hora/data do GPS local do receiver (no momento da emissao)
    GpsTimeData gpsTime = gpsGetTimeData();

    String protocolPacket = buildProtocolPacket(
        team_id, millis_ts, count,
        ax, ay, az, gx, gy, gz,
        temp, press, hum, altp,
        vz, max_alt, state,
        lat, lon, alt, sats,
        parachute,
        gpsTime.hhmmss,
        gpsTime.ddmmyyyy,
        p.rxRssi       // RSSI real medido pelo receiver
    );

    // Retransmite via Serial para o Recovery WebUI
    Serial.println(protocolPacket);

    // Salva no LittleFS com timestamp local
    String logLine = String(millis()) + "," + protocolPacket;
    appendLog(logLine);
}

/**
 * @brief Tenta fechar cestos: emite corrigido quando ha ouvintes
 * suficientes; emite com GPS cru quando o prazo estoura.
 */
static void processBasket() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < TRILAT_BASKET_SLOTS; i++) {
        PendingPacket &p = basket[i];
        if (!p.inUse || p.emitted) continue;

        // 1) Cesto fechou: ouvintes suficientes do MESMO pacote
        if (p.nSamples >= TRILAT_MIN_LISTENERS) {
            // Origem do plano local = primeiro ouvinte (o receiver, "RX",
            // quando tem GPS valido; senao o primeiro beacon da lista)
            double lat0 = p.samples[0].lat;
            double lon0 = p.samples[0].lon;
            TrilaterationResult tri = trilaterate(p.samples, p.nSamples, lat0, lon0);

            if (tri.valid && tri.residualM <= TRILAT_MAX_RESIDUAL_M) {
                p.corrected = true;
                p.emitted   = true;
                float triLat = (float)tri.lat;
                float triLon = (float)tri.lon;
                emitProtocolPacket(p, &triLat, &triLon);
                Serial.println("[TRI] pkt#" + String(p.count)
                               + " corrigido: " + String(triLat, 6) + ","
                               + String(triLon, 6)
                               + " | ouvintes=" + String(p.nSamples)
                               + " | residuo=" + String(tri.residualM, 1) + "m");
                continue;
            }
            if (tri.valid) {
                Serial.println("[TRI] pkt#" + String(p.count)
                               + " residuo alto (" + String(tri.residualM, 0)
                               + "m > " + String(TRILAT_MAX_RESIDUAL_M)
                               + "m) — GPS cru do satellite");
            }
            // Geometria degenerada (colinear) — cai no prazo e emite cru
        }

        // 2) Prazo do cesto estourou: emite com o GPS cru do satellite
        if (now - p.rxMillis >= TRILAT_BASKET_WAIT_MS) {
            p.corrected = false;
            p.emitted   = true;
            if (p.nSamples > 0 && p.nSamples < TRILAT_MIN_LISTENERS) {
                Serial.println("[TRI] pkt#" + String(p.count)
                               + " cesto incompleto (" + String(p.nSamples)
                               + "/" + String(TRILAT_MIN_LISTENERS)
                               + " ouvintes) — GPS cru");
            }
            emitProtocolPacket(p, nullptr, nullptr);  // GPS cru do pacote
        }
    }
}

/**
 * @brief Log periodico do estado do mesh (beacons ativos / ouvindo).
 */
static void logMeshStatus() {
    uint32_t now = millis();
    uint8_t active = 0, listening = 0;
    for (uint8_t i = 0; i < beaconCount; i++) {
        BeaconStatus &b = beacons[i];
        if (now - b.lastSeen < BEACON_ACTIVE_WINDOW_MS) {
            active++;
            // "Ouvindo o satellite": a ultima report referencia um pacote
            // ouvido ha pouco (idade no relogio do beacon)
            int32_t age = (int32_t)(b.lastMillis - b.lastSatMillis);
            if (b.lastSatMillis != 0 && age >= 0
                && age <= (int32_t)BEACON_REPORT_MAX_AGE_MS) {
                listening++;
            }
        }
    }
    Serial.println("[MESH] Beacons ativos: " + String(active)
                   + "/" + String(beaconCount)
                   + " | ouvindo o satellite: " + String(listening)
                   + " | ultimo pkt: #" + String(lastCount));
}

/**
 * @brief Atualiza contadores de pacotes perdidos e loga na Serial.
 *
 * Compara o count recebido com o anterior para detectar saltos.
 * No primeiro pacote, apenas inicializa o contador.
 */
static void trackPacketCount(uint32_t currentCount) {
    totalReceived++;

    if (firstPacket) {
        lastCount = currentCount;
        firstPacket = false;
        Serial.println("[STATS] Primeiro pacote recebido — count=" + String(currentCount));
        return;
    }

    // Detecta salto no contador
    if (currentCount == lastCount + 1) {
        // Sem perda — silencioso para nao poluir a Serial
    } else if (currentCount > lastCount + 1) {
        uint32_t lost = currentCount - lastCount - 1;
        totalLost += lost;
        Serial.println("[LOST] " + String(lost) + " pacote(s) perdido(s) — count "
                       + String(lastCount) + " -> " + String(currentCount)
                       + " | total perdidos: " + String(totalLost));
    } else {
        // count voltou (overflow ou reboot do satellite)
        Serial.println("[STATS] Count resetado — " + String(lastCount)
                       + " -> " + String(currentCount)
                       + " (possivel reboot do satellite)");
    }

    lastCount = currentCount;
}

/**
 * @brief Loga estatisticas acumuladas na Serial.
 */
static void logStats() {
    Serial.println("[STATS] Recebidos: " + String(totalReceived)
                   + " | Perdidos: " + String(totalLost)
                   + " | Erros parse: " + String(totalErrors)
                   + " | Taxa perda: "
                   + String(totalReceived + totalLost > 0
                            ? (totalLost * 100) / (totalReceived + totalLost)
                            : 0)
                   + "%");
}

// ── LittleFS ────────────────────────────────────

static bool setupLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] Falha ao montar LittleFS");
        return false;
    }
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

// ── Handlers de pacotes ─────────────────────────

/**
 * @brief Processa um pacote vindo do satellite (qualquer TEAM_ID).
 *
 * Para o satellite (#213), o pacote entra no cesto de sincronizacao: a
 * emissao para o WebUI espera os reports dos beacons (ate TRILAT_BASKET_WAIT_MS)
 * e sai com lat/lon corrigidos por trilateracao quando possivel.
 * Demais dispositivos (#11, #51) seguem o fluxo antigo: emissao imediata.
 */
static void handleSatellitePacket(const String &raw, int rxRssi) {
    String team_id;
    uint32_t millis_ts, count;
    float ax, ay, az, gx, gy, gz;
    float temp, press, hum, altp;
    float vz, max_alt;
    int state;
    float lat, lon, alt;
    uint8_t sats;
    int parachute;
    int rssi_placeholder;

    if (!parseSatellitePacket(raw, team_id, millis_ts, count,
                              ax, ay, az, gx, gy, gz,
                              temp, press, hum, altp,
                              vz, max_alt, state,
                              lat, lon, alt, sats, parachute, rssi_placeholder)) {
        totalErrors++;
        Serial.println("[STATS] Erro de parse #" + String(totalErrors));
        logStats();
        return;
    }

    // Rastreia pacotes perdidos
    trackPacketCount(count);

    // Trilateracao e' SOMENTE para o satellite (Mission ID #213)
    if (team_id != SAT_TEAM_ID) {
        // Fluxo antigo: emite imediatamente
        PendingPacket tmp;
        memset(&tmp, 0, sizeof(tmp));
        tmp.raw       = raw;
        tmp.count     = count;
        tmp.rxRssi    = rxRssi;
        tmp.inUse     = true;
        tmp.emitted   = true;
        emitProtocolPacket(tmp, nullptr, nullptr);
        return;
    }

    // ── Satellite: entra no cesto de sincronizacao ──
    uint32_t now = millis();
    uint8_t slot = count % TRILAT_BASKET_SLOTS;
    PendingPacket &p = basket[slot];

    // Slot ocupado por outro pacote ainda nao emitido: libera com GPS cru
    if (p.inUse && !p.emitted && p.count != count) {
        Serial.println("[TRI] Slot #" + String(slot) + " sobrescrito (pkt#"
                       + String(p.count) + " -> #" + String(count) + ")");
        p.corrected = false;
        p.emitted   = true;
        emitProtocolPacket(p, nullptr, nullptr);
    }

    p.inUse     = true;
    p.emitted   = false;
    p.corrected = false;
    p.count     = count;
    p.rxMillis  = now;
    p.rxRssi    = rxRssi;
    p.satAltp   = altp;
    p.nSamples  = 0;
    p.raw       = raw;
    if (p.raw.endsWith("#")) {
        p.raw = p.raw.substring(0, p.raw.length() - 1);
    }

    // O proprio receiver e' um ouvinte (quando tem GPS valido)
    GpsData gps = gpsGetData();
    if (gps.valid) {
        ListenerSample &s = p.samples[p.nSamples++];
        strncpy(s.id, "RX", sizeof(s.id));
        s.lat  = gps.lat;
        s.lon  = gps.lon;
        s.altM = (float)gps.altMeters;
        s.rssi = (float)rxRssi;
        s.rangeM = rssiToGroundRange(rxRssi, TRILAT_TX_POWER_DBM,
                                     TRILAT_PATH_LOSS_N, altp - s.altM);
    }

    processBasket();
}

/**
 * @brief Processa um report de beacon.
 *
 * Atualiza a tabela de beacons (check ativos/ouvindo) e, se o report
 * referencia um pacote #213 que ainda esta no cesto, adiciona a amostra
 * RSSI+GPS para a trilateracao.
 */
static void handleBeaconReport(const String &raw, int rxRssi) {
    char id[8];
    BeaconReportRx r;

    if (!parseBeaconReport(raw, id, sizeof(id), r)) {
        Serial.println("[BEACON] Report malformado ignorado: "
                       + raw.substring(0, 40));
        return;
    }

    upsertBeacon(id, r);

    // Idade do pacote ouvido (relogio do beacon — mesmo clock, sem sync)
    int32_t age = (int32_t)(r.millis - r.sat_millis);

    // Usa a amostra so se: referencia um pacote, GPS valido, RSSI fresco
    if (r.sat_millis != 0 && r.gpsv
        && age >= 0 && age <= (int32_t)BEACON_REPORT_MAX_AGE_MS) {
        uint8_t slot = r.sat_count % TRILAT_BASKET_SLOTS;
        PendingPacket &p = basket[slot];
        if (p.inUse && !p.emitted && p.count == r.sat_count) {
            addSample(p, id, r.lat, r.lon, r.alt, (float)r.rssi);
            processBasket();
        }
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(400);
    Serial.println();
    Serial.println("=== Recovery System — LoRa Receiver (Mesh + Trilateracao) ===");
    Serial.println();

    gpsInit();

    loraReady = loraInit();

    fsReady = setupLittleFS();
    if (fsReady) {
        logPath = generateLogPath();
        writeHeader(logPath.c_str());
        Serial.printf("[FS] Log gravando em: %s\n", logPath.c_str());
    }

    Serial.println();
    Serial.println("[SYS] Aguardando pacotes LoRa do satellite...");
    Serial.println("[SYS] Trilateracao: min " + String(TRILAT_MIN_LISTENERS)
                   + " ouvintes | janela do cesto "
                   + String(TRILAT_BASKET_WAIT_MS) + "ms");
    Serial.println();
}

void loop() {
    gpsProcess();

    if (!loraReady || !loraAvailable()) {
        delay(10);
        return;
    }

    String raw = loraReceive();
    if (raw.length() == 0) return;

    int rxRssi = loraLastRSSI();
    Serial.print("[LoRa] RX (RSSI=" + String(rxRssi) + "): ");
    Serial.println(raw);

    // Dispatch por TEAM_ID: report de beacon (#Bx) ou pacote de telemetria
    if (raw.startsWith("#B")) {
        handleBeaconReport(raw, rxRssi);
    } else {
        handleSatellitePacket(raw, rxRssi);
    }

    // Estado do mesh a cada ~5s
    static uint32_t lastMeshLog = 0;
    if (millis() - lastMeshLog >= 5000) {
        lastMeshLog = millis();
        logMeshStatus();
    }
}