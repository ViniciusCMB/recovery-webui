# Protocolo de Comunicação LoRa

Este documento detalha o protocolo de comunicação entre os dispositivos transmissores (foguete/satélite) e o receptor LoRa.

## Índice

- [Visão Geral](#visão-geral)
- [Especificações Técnicas](#especificações-técnicas)
- [Formato dos Pacotes](#formato-dos-pacotes)
- [Campos de Dados](#campos-de-dados)
- [Identificadores de Dispositivos](#identificadores-de-dispositivos)
- [Exemplos de Pacotes](#exemplos-de-pacotes)
- [Tratamento de Erros](#tratamento-de-erros)
- [Mesh de Trilateração](#mesh-de-trilateração)
- [Boas Práticas](#boas-práticas)

## Visão Geral

O sistema utiliza comunicação LoRa (Long Range) para transmitir dados de telemetria dos dispositivos para a estação base. Os dados são formatados em CSV e transmitidos via UART/Serial.

```
┌─────────────────┐          ┌──────────────┐          ┌─────────────────┐
│   Foguete/Sat   │          │   Módulo     │          │   Computador    │
│   (Transmissor) │  LoRa    │   Receptor   │   USB    │   (Recovery     │
│                 │ ────────>│   LoRa       │ ────────>│    WebUI)       │
└─────────────────┘   433MHz └──────────────┘  Serial  └─────────────────┘
```

## Especificações Técnicas

### Parâmetros LoRa

| Parâmetro        | Valor   | Descrição                                                |
| ---------------- | ------- | -------------------------------------------------------- |
| Frequência       | 433 MHz | Frequência ISM (varia por região)                        |
| Bandwidth        | 125 kHz | Largura de banda                                         |
| Spreading Factor | 7-12    | Fator de espalhamento (configura alcance vs. velocidade) |
| Coding Rate      | 4/5     | Taxa de correção de erros                                |
| TX Power         | 20 dBm  | Potência de transmissão                                  |
| Alcance típico   | 2-10 km | Depende do ambiente e antenas                            |

### Comunicação Serial

| Parâmetro    | Valor      |
| ------------ | ---------- |
| Baudrate     | 115200 bps |
| Data bits    | 8          |
| Parity       | None       |
| Stop bits    | 1          |
| Flow control | None       |
| Encoding     | UTF-8      |

## Formato dos Pacotes

### Estrutura Geral

Cada pacote e uma linha de texto em formato CSV terminada com `\n`.

**Formato v2.0 (24 campos) — receptor para backend:**

```
TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi
```

**Formato do satelite #213 (Helike, 18 campos) — transmissor LoRa:**

```
#213,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,alt,lat,lon,sat,rssi#
```

O satelite usa nomes `gp/gr/gy` (giroscopio X/Y/Z) e `ap/ar/ay` (acelerometro
X/Y/Z) e **nao transmite** `vz`, `maxAltitude`, `state` nem `parachute` — o
receiver preenche esses campos com zero ao emitir o formato v2.0 (24 campos)
para o backend. O `#` final marca o fim de pacote RF.

**Formato dos foguetes #11/#51 (flight-computer v2.0, 22 campos) — LoRa:**

```
TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,maxAltitude,state,alt,lat,lon,sat,parachute,rssi
```

O receiver aceita os dois formatos e roteia por TEAM_ID: `#213` → cesto de
trilateração; `#11`/`#51` → retransmissão direta.

**Formato do report de beacon (11 campos) — beacon para receiver:**

```
#B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
```

| #  | Campo       | Tipo    | Unidade | Descricao                                                  |
| -- | ----------- | ------- | ------- | ---------------------------------------------------------- |
| 1  | TEAM_ID     | string  | -       | `#B1`..`#B8` — report de beacon (distinto de pacote de veiculo) |
| 2  | millis      | uint32  | ms      | Uptime do beacon no momento do TX                          |
| 3  | count       | uint32  | -       | Contador sequencial de TX do beacon (debug/perda)          |
| 4  | sat_count   | uint32  | -       | Count do ultimo pacote do satellite ouvido (0 = heartbeat) |
| 5  | sat_millis  | uint32  | ms      | Uptime do beacon quando ouviu esse pacote                  |
| 6  | lat         | float   | °       | Latitude GPS do beacon                                     |
| 7  | lon         | float   | °       | Longitude GPS do beacon                                    |
| 8  | alt         | float   | m       | Altitude GPS do beacon                                     |
| 9  | sat         | uint8   | -       | Satelites GPS em vista no beacon                           |
| 10 | rssi        | int     | dBm     | RSSI do pacote do satellite ouvido (-127 no heartbeat)     |
| 11 | gpsv        | uint8   | -       | 1 = fix GPS valido, 0 = sem fix                            |

Os campos `hora` e `data` nao sao transmitidos pelo satelite (economia de bytes).
O receptor LoRa preenche esses campos com dados do seu GPS local ao retransmitir
via Serial no formato completo de 24 campos.

**Marcador de integridade (simulador apenas):**

O firmware `simulate-receiver.ino` adiciona `#` ao final do pacote (`...rssi#\n`) para
simular o marcador de fim de pacote usado no protocolo RF. O parser Python aceita
ambos os formatos (com ou sem `#` final).

### Caracteristicas

- **Delimitador**: Virgula (`,`)
- **Terminador**: Line Feed (`\n`) ou `#\n` (simulador)
- **Total de campos**: 24 (completo) / 22 (satelite)
- **Tamanho tipico**: ~160 bytes (completo) / ~140 bytes (satelite)
- **Frequencia**: 1-10 Hz

## Campos de Dados

### Tabela Completa (v2.0)

| #  | Campo        | Tipo   | Unidade  | Faixa          | Descricao                                     | Exemplo     |
| -- | ------------ | ------ | -------- | -------------- | --------------------------------------------- | ----------- |
| 1  | TEAM_ID      | string | -        | -              | Identificador do dispositivo                  | #213        |
| 2  | millis       | uint32 | ms       | 0 - 4294967295 | Tempo desde boot do microcontrolador          | 12345       |
| 3  | count        | uint16 | -        | 0 - 65535      | Contador sequencial de pacotes                | 42          |
| 4  | altp         | float  | m        | -500 - 50000   | Altitude barometrica                          | 150.5       |
| 5  | temp         | float  | °C       | -40 - 85       | Temperatura ambiente                          | 25.3        |
| 6  | umi          | float  | %        | 0 - 100        | Umidade relativa do ar                        | 45.2        |
| 7  | p            | float  | hPa      | 300 - 1100     | Pressao atmosferica                           | 1013.25     |
| 8  | gx           | float  | °/s      | -2000 - 2000   | Giroscopio X (pitch)                          | 0.5         |
| 9  | gy           | float  | °/s      | -2000 - 2000   | Giroscopio Y (roll)                           | 1.2         |
| 10 | gz           | float  | °/s      | -2000 - 2000   | Giroscopio Z (yaw)                            | -0.3        |
| 11 | ax           | float  | m/s²     | -156 - 156     | Acelerometro X                                | 0.1         |
| 12 | ay           | float  | m/s²     | -156 - 156     | Acelerometro Y                                | 0.2         |
| 13 | az           | float  | m/s²     | -156 - 156     | Acelerometro Z                                | 9.8         |
| 14 | vz           | float  | m/s      | -500 - 500     | Velocidade vertical (derivada da altp)        | 25.3        |
| 15 | maxAltitude  | float  | m        | -500 - 50000   | Altitude maxima registrada no voo             | 850.0       |
| 16 | state        | uint8  | -        | 0 - 5          | Estado do computador de bordo (voo)           | 3           |
| 17 | hora         | uint32 | HHMMSS   | 0 - 235959     | Hora do GPS (preenchida pelo receptor)        | 143045      |
| 18 | data         | uint32 | DDMMYYYY | -              | Data do GPS (preenchida pelo receptor)        | 20012025    |
| 19 | alt          | float  | m        | -500 - 50000   | Altitude GPS                                  | 150.0       |
| 20 | lat          | float  | °        | -90 - 90       | Latitude (graus decimais)                     | -23.5505    |
| 21 | lon          | float  | °        | -180 - 180     | Longitude (graus decimais)                    | -46.6333    |
| 22 | sat          | uint8  | -        | 0 - 255        | Numero de satelites GPS                       | 8           |
| 23 | parachute    | uint8  | -        | 0 - 1          | Estado do paraquedas (0=recolhido, 1=aberto)  | 0           |
| 24 | rssi         | int8   | dBm      | -120 - 0       | Intensidade sinal LoRa no receptor            | -75         |

### Detalhamento dos Campos

#### TEAM_ID

```
Formato: #XXX
Valores validos:
  #11  - Foguete (Dedalo / Serra Rocketry)
  #51  - Foguete secundario
  #213 - Satellite (Helike PocketQube)
```

#### millis

```
Tempo em milissegundos desde que o microcontrolador iniciou
Overflow: ~49 dias
Uso: Sincronização temporal, cálculo de intervalo entre pacotes
```

#### count

```
Contador incremental de pacotes transmitidos
Uso: Detectar perda de pacotes, ordenação
```

#### altp (Altitude Barométrica)

```
Calculada a partir da pressão atmosférica
Fórmula típica: h = 44330 * (1 - (P/P0)^0.1903)
Onde: P0 = 1013.25 hPa (pressão ao nível do mar)
Precisão: ±1-2 metros
```

#### Dados IMU (gx, gy, gz, ax, ay, az)

```
IMU tipica: MPU6050, BMI088, ICM-20948
Taxa de amostragem: 100-1000 Hz
Dados enviados: Ultima leitura antes da transmissao
```

#### GPS (hora, data, alt, lat, lon, sat)

```
Módulos típicos: NEO-6M, NEO-7M, NEO-M8N
Formato hora: HHMMSS (UTC)
Formato data: DDMMYYYY
Coordenadas: Graus decimais (WGS84)
```

#### rssi

```
Medido no receptor LoRa
Valores típicos:
  -30 a -50 dBm: Excelente
  -50 a -70 dBm: Bom
  -70 a -90 dBm: Razoável
  -90 a -110 dBm: Fraco
  < -110 dBm: Muito fraco/perda iminente
```

## Identificadores de Dispositivos

### TEAM_ID #11 — Foguete Principal (Dedalo / Serra Rocketry)

**Caracteristicas:**

- Computador de bordo principal (ESP32 / Arduino)
- Deteccao de apogeu
- Acionamento de paraquedas
- Sensores: GPS, IMU, Barometro

**Dados especificos:**

- Alta taxa de amostragem durante voo ativo
- Acionamento de paraquedas (pqd)

### TEAM_ID #51 — Foguete Secundario

**Caracteristicas:**

- Segundo veiculo de lancamento
- Sensores: GPS, IMU, Barometro
- Rastreamento independente

### TEAM_ID #213 — Satellite (Helike PocketQube)

**Caracteristicas:**

- Microsatelite / PocketQube (ESP32-C3)
- Sensores ambientais (temp, umi, pressao, IMU, GPS)
- Rastreamento independente
- Sem atuadores de recuperacao

**Dados especificos:**

- Dados de IMU (acelerometro + giroscopio) relevantes
- Dados ambientais (temperatura, umidade, pressao)

### TEAM_ID #B1..#B8 — Beacons (mesh de trilateracao)

**Caracteristicas:**

- Nos terrestres do ground segment (mesmo hardware do receiver)
- Escutam o satellite via LoRa e medem o RSSI do pacote
- Reportam posicao GPS propria + RSSI para o receiver (formato 11 campos)

**Dados especificos:**

- RSSI do mesmo pacote do satellite em varios pontos do campo
- Posicao GPS de cada ouvinte — base para a trilateracao


## Exemplos de Pacotes (v2.0)

### Foguete em Solo (Antes do Lancamento)

```
#11,5000,1,150.5,25.3,45.2,1013.25,0.1,0.2,-0.1,0.0,0.1,9.8,0.0,150.5,0,143000,20012025,150.0,-23.5505,-46.6333,8,0,-45
```

**Analise:**

- Tempo: 5 segundos desde boot
- Pacote #1
- Altitude: 150.5m (nivel do solo local)
- Aceleracao Z: 9.8 m/s² (gravidade - foguete parado verticalmente)
- Satelites: 8 (GPS fixo)
- Paraquedas: Fechado (0)
- RSSI: -45 dBm (excelente)

### Foguete em Ascensao

```
#11,15000,30,450.2,20.1,48.5,950.30,150.5,45.2,30.1,5.2,15.3,80.5,35.7,450.2,1,143015,20012025,445.0,-23.5505,-46.6333,7,0,-65
```

**Analise:**

- Tempo: 15 segundos desde boot (10s de voo)
- Pacote #30
- Altitude: 450m (300m acima do solo)
- Velocidade vertical: 35.7 m/s (subindo)
- Estado: 1 (propulsao ativa)
- Paraquedas: Fechado (0)

### Foguete em Apogeu

```
#11,25000,50,850.0,15.5,50.2,900.15,5.2,2.1,1.5,0.1,-0.2,9.8,0.2,850.0,3,143025,20012025,845.0,-23.5510,-46.6330,6,1,-80
```

**Analise:**

- Tempo: 25 segundos
- Altitude maxima: 850m
- Velocidade vertical: ~0 m/s (apogeu)
- Estado: 3 (apogeu — paraquedas acionado)
- Paraquedas: Aberto (1)

### Foguete em Descida

```
#11,45000,90,200.0,24.8,46.0,1010.50,2.1,1.5,0.8,0.2,0.3,12.5,-12.3,850.0,4,143045,20012025,195.0,-23.5520,-46.6325,8,1,-55
```

**Analise:**

- Tempo: 45 segundos
- Altitude: 200m (descendo)
- Velocidade vertical: -12.3 m/s (descida controlada pelo paraquedas)
- Paraquedas: Aberto (1)

### Satellite em Orbita

```
#213,10000,15,500.0,28.5,55.0,955.0,0.02,-0.01,0.01,0.05,0.10,-9.81,0.00,500.0,0,143010,22072026,500.0,-21.94305,-48.95409,10,0,-50
```

**Analise:**

- Satelite #213 (Helike PocketQube)
- Altitude: 500m (orbita baixa simulada)
- Temperatura: 28.5°C (ambiente controlado)
- GPS com 10 satelites
- Sem paraquedas (0)

## Tratamento de Erros

### Pacotes Incompletos

**Problema:** Pacote recebido com menos de 24 campos

**Causa:**

- Interferencia no sinal
- Pacote cortado
- Erro de transmissao

**Acao do servidor:**

```python
result = parse_packet(raw)
if result is None:
    antenna_logger.warning(
        f"parse falhou: {len(fields)} campos (esperado 24)"
    )
    continue
team_id, fields = result
```

### Dados Inválidos

**Validação recomendada (firmware):**

```cpp
// Validar coordenadas GPS
if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
    // Coordenadas inválidas, usar último valor válido
}

// Validar altitude
if (alt < -500 || alt > 50000) {
    // Altitude suspeita
}

// Validar temperatura
if (temp < -40 || temp > 85) {
    // Sensor com problema
}
```

### Perda de Pacotes

**Detecção:**

```python
last_count = 0
lost_packets = 0

if count != last_count + 1:
    lost = count - last_count - 1
    lost_packets += lost
    print(f"⚠️  {lost} pacotes perdidos!")

last_count = count
```

## Mesh de Trilateração

### Objetivo

Quando o satellite está longe do alcance de visada ou o GPS dele falha/deriva,
o receiver recalcula a posição projetada no chão usando o **RSSI do mesmo
pacote** medido por vários ouvintes (receiver + beacons).

### Fluxo

```
Satellite (#213, 5 Hz)
   │  LoRa
   ▼
Receiver (~origem) ──────┐       Beacon B1..B8 (GPS proprio)
   │  escuta + RSSI       │       │  escuta + RSSI
   │                      │       ▼
   ▼                      │  report #Bx (11 campos, jitter 15-120 ms)
cesto de sincronização ◄──┴───────┘
(count % 8, janela 900 ms)
   │
   ├─ ≥3 ouvintes ├─ LSQ 2D (residuo ≤ 1500 m) ─► emite #213 corrigido
   │
   └─ prazo estourado ─► emite #213 com GPS cru do pacote
```

### Modelo RSSI → distância

```
r = 10 ^ ((TX_POWER - RSSI) / (10 * n))     [slant range, m]
d = sqrt(r² - h²)                           [projeção no chão, h = altp do pacote]
```

- `TX_POWER` = 20 dBm (potência do satellite Helike — `LORA_TX_POWER` no config.h)
- `n` = 2.0 (espaço livre; ajustável no receiver em `TRILAT_PATH_LOSS_N`)

### Regras de validação

1. Report `#Bx` só vira amostra se: referencia um pacote (`sat_count != 0`),
   beacon tem fix GPS (`gpsv == 1`) e o RSSI é fresco (`millis - sat_millis ≤ 1500 ms`).
2. Correção aplicada **somente** ao satellite `#213` — foguetes `#11`/`#51`
   seguem retransmissão direta.
3. Solução LSQ com resíduo médio > 1500 m é descartada (GPS cru prevalece).

### Precisão (Monte Carlo, mesma fórmula do firmware)

| Ruído RSSI | Erro mediano | p95    |
|------------|--------------|--------|
| ±1 dB      | 134 m        | 309 m  |
| ±2 dB      | 268 m        | 744 m  |

**Leitura**: a correção é um refinamento grosseiro — com RSSI ruidoso o GPS
cru do satellite continua sendo a fonte primária. O encaixe é usado como
cross-check e fallback quando o GPS do satellite falha.

## Boas Práticas

### Firmware (Transmissor)

1. **Buffering**: Mantenha buffer de pacotes não transmitidos
2. **Retry**: Implemente retransmissão de pacotes críticos
3. **Checksum**: Considere adicionar campo de verificação
4. **Compressão**: Para longas distâncias, comprima dados
5. **Priorização**: Envie dados críticos (GPS, altitude) com maior frequência

### Receptor

1. **Timeout**: Configure timeout adequado (0.5-1s)
2. **Validação**: Valide formato antes de processar
3. **Logging**: Registre TODOS os pacotes (incluindo inválidos)
4. **Timestamp**: Adicione timestamp do servidor
5. **Backup**: Faça backup periódico dos logs

### Exemplo de Código Transmissor (Arduino)

```cpp
void sendTelemetry() {
    char buffer[256];

    snprintf(buffer, sizeof(buffer),
        "#11,%lu,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%lu,%lu,%.2f,%.6f,%.6f,%u,%d,%d\n",
        millis(),           // millis
        packetCount++,      // count
        altitude,           // altp
        temperature,        // temp
        humidity,           // umi
        pressure,           // p
        gyro.x, gyro.y, gyro.z,   // gx, gy, gz
        accel.x, accel.y, accel.z, // ax, ay, az
        verticalSpeed,      // vz
        maxAltitude,        // maxAltitude
        flightState,        // state
        gpsTime,            // hora
        gpsDate,            // data
        gpsAltitude,        // alt
        latitude,           // lat
        longitude,          // lon
        satellites,         // sat
        parachuteState,     // parachute
        lora.getRSSI()      // rssi
    );

    LoRa.print(buffer);
}
```

## Troubleshooting

### Sinal Fraco (RSSI baixo)

**Soluções:**

- Melhore as antenas (use antenas de ganho)
- Aumente TX power (máximo 20 dBm)
- Aumente Spreading Factor (SF8-SF12)
- Reduza bandwidth
- Verifique obstáculos entre transmissor e receptor

### Alta Taxa de Perda de Pacotes

**Soluções:**

- Reduza taxa de transmissão
- Aumente Coding Rate (4/6, 4/7)
- Verifique interferência em 433 MHz
- Use canal diferente se disponível

### Coordenadas GPS Incorretas

**Causas:**

- GPS sem fix (sat < 4)
- Dados não atualizados
- Módulo GPS com problema

**Solução:**

```cpp
// Apenas envie se GPS válido
if (gps.satellites.value() >= 4 && gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
}
```

---

## Referências

- [LoRa Alliance](https://lora-alliance.org/)
- [Semtech LoRa Documentation](https://www.semtech.com/lora)
- [Arduino LoRa Library](https://github.com/sandeepmistry/arduino-LoRa)

---

[← Voltar à Documentação](README.md)
