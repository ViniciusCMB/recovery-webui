#pragma once
#include <Arduino.h>

// Driver LoRa do beacon.
//
// Alem de transmitir (loraSend), o beacon agora TAMBEM escuta o satellite
// (loraAvailable/loraReceive): ele precisa ouvir o pacote do satellite para
// reportar ao receiver o count do pacote e o RSSI medido. O radio fica em
// modo RX continuo e apenas pausa durante a propria transmissao.

bool loraInit();                      // Inicializa LoRa (freq, sync word, etc.) em modo RX
bool loraSend(const String& payload); // Envia pacote LoRa e volta para RX
bool loraAvailable();                 // true se ha pacote recebido pronto
String loraReceive();                 // Le o pacote recebido (bloqueia ate' fim do RX)
int  loraLastRSSI();                  // RSSI do ultimo pacote recebido (dBm)