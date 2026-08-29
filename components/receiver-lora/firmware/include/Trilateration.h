#pragma once

/**
 * @file Trilateration.h
 * @brief Trilateracao GPS a partir de amostras RSSI de multiplos ouvintes.
 *
 * Pipeline (mesh de beacons + receiver):
 *  1. Cada ouvinte (receiver ou beacon) mede o RSSI do MESMO pacote do
 *     satellite (sincronizado pelo campo `count` do pacote).
 *  2. RSSI -> distancia obliqua:   r = 10^((TX_POWER - RSSI)/(10*n))
 *  3. Correcao de altitude (Pitagoras): d_chao = sqrt(r^2 - h^2), onde h e'
 *     a altitude do satellite acima do ouvinte.
 *  4. Coordenadas GPS dos ouvintes -> plano cartesiano (aproximacao
 *     equirretangular, valida para < 10 km) com origem no receiver.
 *  5. Solucao dos minimos quadrados do sistema linear A*x = b (2x2,
 *     fechada — sem dependencias externas).
 *  6. (x, y) -> lat/lon corrigidos do satellite.
 */

#include <stdint.h>
#include <math.h>

/** Numero maximo de ouvintes suportado por chamada de trilaterate(). */
#define TRILAT_MAX_SAMPLES  16

/** Amostra de um ouvinte para um dado pacote do satellite. */
struct ListenerSample {
    char    id[8];    // "RX" (receiver) ou "B1".."B8" (beacon)
    double  lat;      // posicao do ouvinte (graus decimais)
    double  lon;
    float   altM;     // altitude do ouvinte (m)
    float   rssi;     // RSSI do pacote do satellite medido pelo ouvinte (dBm)
    double  rangeM;   // distancia projetada no chao (resultado do passo 2+3)
};

/** Resultado da trilateracao. */
struct TrilaterationResult {
    bool   valid;     // false se geometria degenerada (ouvintes colineares)
    double lat;       // latitude corrigida do satellite (graus)
    double lon;       // longitude corrigida do satellite (graus)
    double xM;        // posicao no plano local (m, origem = ouvinte 0)
    double yM;
    float  residualM; // residuo medio (m) — qualidade do ajuste
    uint8_t listeners;// numero de ouvintes usados na solucao
};

/**
 * @brief Converte RSSI em distancia projetada no chao.
 *
 * @param rssi     RSSI medido (dBm, negativo)
 * @param txPowerDbm Potencia de transmissao do satellite (dBm)
 * @param pathLossN  Expoente de perda de percurso (2.0 = espaco livre)
 * @param altM     Altitude do satellite acima do ouvinte (m) — 0 desativa
 *                 a correcao de Pitagoras
 */
double rssiToGroundRange(float rssi, float txPowerDbm, float pathLossN,
                         float altM);

/**
 * @brief Resolve a posicao do satellite por minimos quadrados.
 *
 * @param samples Amostras dos ouvintes (>= 3). samples[0] e' a origem do
 *                plano local (tipicamente o receiver).
 * @param n       Numero de amostras
 * @param lat0    Latitude da origem (graus)
 * @param lon0    Longitude da origem (graus)
 */
TrilaterationResult trilaterate(const ListenerSample *samples, uint8_t n,
                                double lat0, double lon0);