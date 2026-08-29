#pragma once

/**
 * @file payload.h
 * @brief Formatacao de pacotes para retransmissao via Serial
 *
 * O receiver recebe pacotes LoRa do satellite no formato v2.0 (22 campos CSV,
 * ver firmware do flight-computer / docs/telemetry-format.md) e retransmite
 * via Serial no formato do protocolo Recovery WebUI (24 campos CSV),
 * inserindo hora/data do GPS local do receiver e o RSSI real do link.
 *
 * Formato do satellite (22 campos, recebido):
 * TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,
 * maxAltitude,state,alt,lat,lon,sat,parachute,rssi
 *
 * Formato do protocolo (24 campos, retransmitido):
 * TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,
 * maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi
 * (hora/data inseridos apos "state"; rssi e' o valor real do receiver)
 */

#include <Arduino.h>

/**
 * @brief Monta linha CSV de 24 campos para o protocolo Recovery WebUI.
 *
 * @param team_id    Identificador (ex: "#213")
 * @param millis_ts  Timestamp ms desde boot
 * @param count      Contador sequencial de pacotes
 * @param ax,ay,az   Acelerometro (m/s2)
 * @param gx,gy,gz   Giroscopio (rad/s)
 * @param temp       Temperatura (C)
 * @param press      Pressao (hPa)
 * @param hum        Umidade (%) — 0 se o satellite nao tem sensor
 * @param altp       Altitude barometrica (m)
 * @param vz         Velocidade vertical (m/s)
 * @param max_alt    Altitude maxima ate o momento (m)
 * @param state      Estado da maquina de estados (int)
 * @param lat        Latitude GPS (graus)
 * @param lon        Longitude GPS (graus)
 * @param alt        Altitude GPS (m)
 * @param sats       Numero de satelites GPS
 * @param parachute  Estado do paraquedas (0/1)
 * @param hora       Hora GPS do receiver (HHMMSS)
 * @param data_gps   Data GPS do receiver (DDMMYYYY)
 * @param rssi       RSSI do sinal LoRa (dBm) — medido pelo receiver
 * @return String CSV terminada com \n
 */
static String buildProtocolPacket(
    const String &team_id,
    uint32_t millis_ts,
    uint32_t count,
    float ax, float ay, float az,
    float gx, float gy, float gz,
    float temp, float press, float hum,
    float altp,
    float vz,
    float max_alt,
    int state,
    float lat, float lon, float alt,
    uint8_t sats,
    int parachute,
    uint32_t hora,
    uint32_t data_gps,
    int rssi
) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "%s,%lu,%u,"
        "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%d,"
        "%lu,%lu,"
        "%.2f,%.6f,%.6f,%u,%d,%d",
        team_id.c_str(), millis_ts, count,
        altp, temp, hum, press,
        gx, gy, gz,
        ax, ay, az,
        vz, max_alt, state,
        hora, data_gps,
        alt, lat, lon, sats,
        parachute, rssi
    );
    return String(buffer);
}
