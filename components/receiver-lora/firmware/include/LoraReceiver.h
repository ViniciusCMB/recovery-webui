#pragma once
#include <Arduino.h>

bool loraInit();                      // Inicializa LoRa (freq, sync word, etc.)
bool loraSend(const String& payload); // Envia pacote LoRa (para debug/ack)
int  loraLastRSSI();                  // RSSI do ultimo pacote recebido

// Modo receptor
bool loraAvailable();                 // Ha pacote LoRa disponivel para leitura?
String loraReceive();                 // Le um pacote LoRa recebido (retorna vazio se nao ha)
