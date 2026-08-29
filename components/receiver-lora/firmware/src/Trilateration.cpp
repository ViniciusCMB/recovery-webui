/**
 * @file Trilateration.cpp
 * @brief Implementacao da trilateracao GPS por RSSI (minimos quadrados 2D).
 */

#include "Trilateration.h"

// Aproximacao equirretangular (valida para distancias < 10 km):
//   X = (lon - lon0) * 111320 * cos(lat0)      [m]
//   Y = (lat - lat0) * 111000                  [m]
static const double M_PER_DEG_LAT      = 111000.0;
static const double M_PER_DEG_LON_EQ   = 111320.0;

double rssiToGroundRange(float rssi, float txPowerDbm, float pathLossN,
                         float altM) {
    // Perda de percurso no espaco livre:
    //   r = 10 ^ ((TxPower - RSSI) / (10 * n))
    double r = pow(10.0, ((double)txPowerDbm - (double)rssi)
                         / (10.0 * (double)pathLossN));

    // Correcao de altitude: r e' a distancia obliqua; projeta no chao.
    if (altM > 0.0f) {
        double h2 = (double)altM * (double)altM;
        double r2 = r * r - h2;
        if (r2 > 1.0) {
            r = sqrt(r2);          // projecao valida
        } else {
            r = 0.0;               // satellite no nadir (ruido do modelo)
        }
    }
    return r;
}

static void toLocalXY(double lat, double lon, double lat0, double lon0,
                      double &x, double &y) {
    double cosLat0 = cos(lat0 * M_PI / 180.0);
    x = (lon - lon0) * M_PER_DEG_LON_EQ * cosLat0;
    y = (lat - lat0) * M_PER_DEG_LAT;
}

TrilaterationResult trilaterate(const ListenerSample *samples, uint8_t n,
                                double lat0, double lon0) {
    TrilaterationResult res;
    res.valid = false;
    res.listeners = n;
    res.residualM = 0.0f;
    res.xM = 0.0;
    res.yM = 0.0;
    res.lat = lat0;
    res.lon = lon0;

    if (n < 3) return res;  // triangulacao minima exigida

    // 1) Converte todos os ouvintes para o plano local (m)
    double x[TRILAT_MAX_SAMPLES], y[TRILAT_MAX_SAMPLES], r[TRILAT_MAX_SAMPLES];
    for (uint8_t i = 0; i < n; i++) {
        toLocalXY(samples[i].lat, samples[i].lon, lat0, lon0, x[i], y[i]);
        r[i] = samples[i].rangeM > 0.0 ? samples[i].rangeM : 0.0;
    }

    // 2) Monta o sistema linear A*[xsat,ysat] = b (diferencas vs ouvinte 0)
    //    Para i=1..n-1:
    //      2*(xi-x0)*xsat + 2*(yi-y0)*ysat =
    //          r0^2 - ri^2 + xi^2 - x0^2 + yi^2 - y0^2
    double m00 = 0.0, m01 = 0.0, m11 = 0.0;   // M = A^T A (2x2)
    double v0  = 0.0, v1  = 0.0;              // v = A^T b

    for (uint8_t i = 1; i < n; i++) {
        double a0 = 2.0 * (x[i] - x[0]);
        double a1 = 2.0 * (y[i] - y[0]);
        double bi = r[0] * r[0] - r[i] * r[i]
                  + x[i] * x[i] - x[0] * x[0]
                  + y[i] * y[i] - y[0] * y[0];

        m00 += a0 * a0;
        m01 += a0 * a1;
        m11 += a1 * a1;
        v0  += a0 * bi;
        v1  += a1 * bi;
    }

    // 3) Resolve M*[xsat,ysat] = v (inversa 2x2 fechada)
    double det = m00 * m11 - m01 * m01;
    if (fabs(det) < 1e-9) return res;  // ouvintes colineares / degenerados

    res.xM = (v0 * m11 - m01 * v1) / det;
    res.yM = (m00 * v1 - v0 * m01) / det;

    // 4) Converte de volta para GPS
    double cosLat0 = cos(lat0 * M_PI / 180.0);
    res.lat = lat0 + res.yM / M_PER_DEG_LAT;
    res.lon = lon0 + res.xM / (M_PER_DEG_LON_EQ * cosLat0);
    res.valid = true;

    // 5) Residuo medio (qualidade do ajuste)
    double sum = 0.0;
    for (uint8_t i = 0; i < n; i++) {
        double dx = res.xM - x[i];
        double dy = res.yM - y[i];
        sum += fabs(sqrt(dx * dx + dy * dy) - r[i]);
    }
    res.residualM = (float)(sum / n);

    return res;
}