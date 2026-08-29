#pragma once

/**
 * @file payload.h
 * @brief Formato do pacote de report do beacon (mesh de trilateracao)
 *
 * O beacon escuta o satellite (#213) e transmite um report com:
 *  - sua propria posicao GPS (lat/lon/alt + fix)
 *  - o count do ULTIMO pacote do satellite que ouviu (sync para o cesto)
 *  - o RSSI medido desse pacote
 *
 * Formato (11 campos CSV, terminal '#' opcional):
 *   #B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
 *
 *   #B1        TEAM_ID do beacon ("#B1".."#B8") — distingue do satellite
 *   millis     uptime do beacon no momento do TX (ms)
 *   count      contador sequencial local de TX do beacon
 *   sat_count  count do ultimo pacote do satellite ouvido
 *              0 quando o beacon nao ouviu o satellite (heartbeat)
 *   sat_millis millis (relogio do beacon) em que esse pacote foi ouvido;
 *              0 no heartbeat. O receiver calcula a idade do RSSI com
 *              (millis - sat_millis) — relogio unico, sem sync de rede.
 *   lat,lon    posicao GPS do beacon (graus decimais; 0.0 sem fix)
 *   alt        altitude GPS do beacon (m)
 *   sat        numero de satelites GPS em vista
 *   rssi       RSSI do pacote do satellite ouvido (dBm); -127 no heartbeat
 *   gpsv       1 se o GPS do beacon tem fix valido, 0 caso contrario
 */

#include <Arduino.h>

struct BeaconReport {
    uint32_t millis;      // uptime do beacon no TX
    uint32_t count;       // contador local de TX
    uint32_t sat_count;   // count do ultimo pacote do satellite ouvido (0 = nao ouve)
    uint32_t sat_millis;  // millis do beacon quando ouviu esse pacote (0 = heartbeat)
    float    lat;         // lat GPS do beacon (0.0 sem fix)
    float    lon;         // lon GPS do beacon (0.0 sem fix)
    float    alt;         // alt GPS do beacon (m)
    uint8_t  sat;         // satelites GPS em vista
    int      rssi;        // RSSI do pacote do satellite ouvido (dBm)
    uint8_t  gpsv;        // 1 = GPS valido (fix), 0 = sem fix
};

static String buildBeaconReport(
    const String &beacon_id,   // ex: "B1"
    const BeaconReport &r
) {
    char buffer[160];
    snprintf(buffer, sizeof(buffer),
        "#%s,%lu,%lu,%lu,%lu,"
        "%.6f,%.6f,%.2f,"
        "%u,%d,%u",
        beacon_id.c_str(),
        r.millis, r.count, r.sat_count, r.sat_millis,
        r.lat, r.lon, r.alt,
        r.sat, r.rssi, r.gpsv
    );
    return String(buffer);
}