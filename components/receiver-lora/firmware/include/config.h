#pragma once

// ── LoRa RFM95W (SPI) ──────────────────────────
#define LORA_SCK        4
#define LORA_MOSI       3
#define LORA_MISO       2
#define LORA_CS         5
#define LORA_RST        6
#define LORA_IRQ        7

// Frequência e sync word devem bater com o transmissor (satellite)
#define LORA_FREQUENCY  915E6   // Hz  (915 MHz Americas/Brasil)
#define LORA_SYNC_WORD  0xF3    // deve bater com satellite
#define LORA_SF         7       // Spreading Factor 7–12
#define LORA_BW         125E3   // Bandwidth Hz
#define LORA_CR         5       // Coding Rate (4/5)
#define LORA_TX_POWER   17      // dBm

// ── GPS (UART1) ─────────────────────────────────
#define GPS_RX_PIN      20
#define GPS_TX_PIN      21
#define GPS_BAUD        9600

// ── Serial (USB) ────────────────────────────────
#define SERIAL_BAUD     115200

// ── Mission ID do satellite ─────────────────────
// A correcao por trilateracao so e aplicada ao satellite (Helike PocketQube).
// Foguetes (#11, #51) continuam com o fluxo antigo (retransmissao direta).
#define SAT_TEAM_ID                 "#213"

// ── Mesh de beacons / trilateracao ──────────────
// Numero minimo de ouvintes (receiver conta como 1) com RSSI do MESMO
// pacote do satellite para fechar o cesto e calcular a correcao.
// Com TRILAT_MIN_LISTENERS=3: receiver + 2 beacons, ou 3 beacons.
#define TRILAT_MIN_LISTENERS        3

// Residuo medio maximo (m) aceito para aplicar a correcao. Um residuo alto
// indica RSSI ruidoso/geometria ruim — a solucao explodiu e o GPS cru do
// satellite e' mais confiavel. (Simulado: com filtro de 1500 m, 0/4000
// trials de ruido +/-2 dB produziram erro > 3 km.)
#define TRILAT_MAX_RESIDUAL_M       1500

// Janela de espera do cesto de sincronizacao: quanto tempo o receiver
// segura o pacote #213 aguardando reports dos beacons antes de emitir.
// Se o cesto fechar (>= MIN_LISTENERS) antes, emite JÁ com posicao
// corrigida; se estourar o prazo, emite com o GPS cru do satellite.
#define TRILAT_BASKET_WAIT_MS       900

// Link budget da trilateracao (RSSI -> distancia):
//   r = 10 ^ ((TX_POWER - RSSI) / (10 * n))
// TX_POWER = potencia de transmissao do satellite (20 dBm no Helike config.h)
// n        = expoente de perda de percurso (2.0 = espaco livre)
#define TRILAT_TX_POWER_DBM         20
#define TRILAT_PATH_LOSS_N          2.0f

// Correcao de altitude (Pitagoras): a distancia derivada do RSSI e a
// distancia obliqua (slant range); projeta no chao usando a altitude do
// satellite do proprio pacote: d_chao = sqrt(r^2 - h^2).
#define TRILAT_ALT_CORRECTION       1

// Limites do mesh
#define TRILAT_MAX_BEACONS          8       // beacons rastreados (B1..B8)
#define TRILAT_BASKET_SLOTS         8       // slots do cesto (count % SLOTS)

// Beacon considerado ATIVO se report recebido ha menos que este intervalo
#define BEACON_ACTIVE_WINDOW_MS     5000

// Idade maxima (relogio do beacon) do pacote do satellite referenciado no
// report para o RSSI ser valido — impede usar RSSI de pacotes antigos.
#define BEACON_REPORT_MAX_AGE_MS    1500