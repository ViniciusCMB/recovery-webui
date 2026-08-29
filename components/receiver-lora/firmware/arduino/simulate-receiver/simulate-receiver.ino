/*
 * Simulate Receiver
 *
 * Gera pacotes de telemetria simulados e printa na Serial
 * como se tivessem sido recebidos via LoRa.
 *
 * Formato: CSV com 22 campos + marcador '#', igual ao flight-computer v2.0
 *   TEAM_ID,millis_ts,count,altp,temp,umi,press,gx,gy,gz,ax,ay,az,
 *   vz,maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi#
 *
 * Serial: 115200 baud
 * Frequencia: 10 Hz
 */

#define SERIAL_BAUD  115200
#define PKT_INTERVAL 100   // 10 Hz

static const char* TEAM_ID = "#213";

static uint32_t _count = 0;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);
    Serial.println(F("[SIM] Iniciando simulacao de recebimento LoRa..."));
}

void loop() {
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last < PKT_INTERVAL) return;
    last = now;

    // Avança contadores
    _count++;

    // Gera valores com pequena variação a cada pacote
    float altp    = 0.0f + (float)(_count % 500) * 2.0f;     // sobe ate ~1000m e reseta
    float temp    = 25.0f + (float)(rand() % 100) / 100.0f;
    float hum     = 60.0f + (float)(rand() % 200) / 100.0f;
    float press   = 950.0f + (float)(rand() % 50) / 100.0f;
    float gx      = (float)(rand() % 200 - 100) / 100.0f;
    float gy      = (float)(rand() % 200 - 100) / 100.0f;
    float gz      = (float)(rand() % 200 - 100) / 100.0f;
    float ax      = (float)(rand() % 200 - 100) / 100.0f;
    float ay      = (float)(rand() % 200 - 100) / 100.0f;
    float az      = -9.8f + (float)(rand() % 100) / 100.0f;
    float vz      = (float)(rand() % 200 - 100) / 100.0f;
    float maxAlt  = 0.0f;
    int   state   = 0;
    unsigned long hora = 143000UL + _count / 10;
    unsigned long data = 22072026UL;
    float alt     = 478.0f + altp;
    float lat     = -21.94305f;
    float lon     = -48.95409f;
    int   sats    = 10 + rand() % 6;
    int   para    = 0;
    int   rssi    = -(50 + rand() % 40);

    // Monta CSV: 23 campos + '#'
    char buf[256];
    snprintf(buf, sizeof(buf),
        "%s,%lu,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%lu,%lu,%.2f,%.6f,%.6f,%u,%d,%d#",
        TEAM_ID, millis(), _count,
        altp, temp, hum, press, gx, gy, gz, ax, ay, az,
        vz, maxAlt, state,
        hora, data, alt, lat, lon, sats, para, rssi);

    Serial.println(buf);
}
