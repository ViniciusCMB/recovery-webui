#pragma once

// ── LoRa RFM95W (SPI) ──────────────────────────
// Mesmos pinos do receiver
#define LORA_SCK        4
#define LORA_MOSI       3
#define LORA_MISO       2
#define LORA_CS         5
#define LORA_RST        6
#define LORA_IRQ        7

#define LORA_FREQUENCY  915E6   // Hz (915 MHz Americas/Brasil)
#define LORA_SYNC_WORD  0xF3    // deve bater com receiver
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

// ── Identidade do beacon ────────────────────────
// Cada beacon do mesh deve ser gravado com um ID unico ("B1".."B8").
// O pacote de report usa "#B1" como TEAM_ID, o que permite ao receiver
// distinguir report de beacon de pacote de satellite ("#213").
#define BEACON_ID       "B1"

// TEAM_ID do satellite que o beacon escuta (Mission ID)
#define SAT_TEAM_ID     "#213"

// ── Temporizacao dos reports ────────────────────
// Intervalo minimo entre reports que referenciam um pacote ouvido do
// satellite. O satellite transmite a 5 Hz; com 250 ms o beacon reporta
// ~4x/s e ainda deixa espaco para os demais beacons no canal.
#define REPORT_MIN_INTERVAL_MS  250

// Heartbeat: quando o beacon NAO ouve o satellite ha mais que este
// intervalo, envia report com sat_count=0 para o receiver saber que o
// beacon esta ativo (mas nao ouvindo).
#define HEARTBEAT_INTERVAL_MS   2000

// Jitter aleatorio aplicado antes de cada TX (anti-colisao): quando
// varios beacons ouvem o MESMO pacote do satellite, sem jitter eles
// transmitiriam o report juntos e colidiriam no receiver.
#define TX_JITTER_MIN_MS        15
#define TX_JITTER_MAX_MS        120