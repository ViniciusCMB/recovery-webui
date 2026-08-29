# Beacon LoRa — Ouvinte + Report (Mesh de Trilateração)

## Visão Geral

O beacon é um nó do **mesh de rastreamento** do ground segment. Ele faz a
*mesma leitura que o receiver* (escuta os pacotes do satellite via LoRa) e,
além disso, transmite para o receiver o seu próprio posicionamento GPS
(lat/lon/alt) e o RSSI do sinal recebido do satellite. Com as amostras de
vários beacons + o receiver, o receiver calcula a **trilateração GPS** e
emite uma lat/lon mais ajustada para o satellite.

Hardware idêntico ao `receiver-lora` (ESP32-C3 + RFM95W + GPS NEO-8M), o que
permite a qualquer momento promover um beacon a receiver e vice-versa.

## Arquitetura

```
                LoRa 915 MHz                          LoRa 915 MHz
Satellite (#213) ────────────────► Beacon ───────────────────────────► Receiver
  (5 Hz, 18 campos)               (escuta + mede RSSI)   (report #Bx,    │
                                                   11 campos GPS+RSSI)   │
                                                              │ trilateração
                                                              ▼
                                                      WebUI (lat/lon corrigida)
```

O beacon **escuta** o satellite o tempo todo (modo RX contínuo), pausando
apenas durante a própria transmissão (~40 ms). Quando ouve um pacote do
satellite, agenda um report com jitter aleatório anti-colisão e o transmite.

## Fluxo de operação

```mermaid
loop:
  1. gpsProcess()                     — alimenta o parser GPS local
  2. loraAvailable() / loraReceive()  — escuta o satellite
  3. parseSatellitePacket()           — valida (18 campos) e confere TEAM_ID == #213
  4. grava sat_count / sat_millis / RSSI do pacote ouvido
  5. scheduleTx()                     — agenda TX com jitter (15-120 ms) e
                                        respeitando REPORT_MIN_INTERVAL_MS
  6. na hora agendada: buildBeaconReport() + loraSend()
```

### Heartbeat

Se o beacon **não ouve** o satellite por mais que `HEARTBEAT_INTERVAL_MS`
(2 s), envia um report com `sat_count=0` e `sat_millis=0`. O receiver usa
isso para o check de beacons: *ativo* (report recente) mas *não ouvindo o
satellite*. Também mantém o GPS do beacon atualizado na tabela do receiver.

### Jitter anti-colisão

Quando vários beacons ouvem o MESMO pacote do satellite, sem jitter eles
transmitiriam o report simultaneamente e colidiriam no receiver. Cada beacon
aplica um atraso aleatório de 15-120 ms antes do TX (jitter com `randomSeed`
de entropia do boot). O cesto do receiver espera até 900 ms, então o jitter
não prejudica a sincronização.

## Formato do Pacote (report #Bx)

```csv
#B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
```

| #  | Campo       | Tipo    | Descrição                                                      |
| -- | ----------- | ------- | -------------------------------------------------------------- |
| 1  | TEAM_ID     | string  | `#B1`..`#B8` — distingue report de beacon de pacote de satellite |
| 2  | millis      | uint32  | uptime do beacon no TX (ms)                                    |
| 3  | count       | uint32  | contador sequencial local de TX (debug/perda)                  |
| 4  | sat_count   | uint32  | count do último pacote do satellite ouvido (0 = heartbeat)     |
| 5  | sat_millis  | uint32  | millis (relógio do beacon) em que ouviu esse pacote            |
| 6  | lat         | float   | latitude GPS do beacon (graus decimais; 0 sem fix)             |
| 7  | lon         | float   | longitude GPS do beacon                                        |
| 8  | alt         | float   | altitude GPS do beacon (m)                                     |
| 9  | sat         | uint8   | satélites GPS em vista                                         |
| 10 | rssi        | int     | RSSI do pacote do satellite ouvido (dBm; -127 no heartbeat)    |
| 11 | gpsv        | uint8   | 1 = fix GPS válido, 0 = sem fix                                |

Exemplo (beacon B1 ouviu o pacote #45 do satellite com -82 dBm):

```csv
#B1,125840,17,45,125790,-21.774200,-48.177500,608.00,9,-82,1
```

> **Sincronização**: os campos `sat_count` + `sat_millis` usam o relógio do
> próprio beacon (sem sync de rede). O receiver calcula a idade do RSSI com
> `millis - sat_millis` e só usa amostras com idade ≤
> `BEACON_REPORT_MAX_AGE_MS` (1,5 s), garantindo que o RSSI seja do *mesmo*
> pacote em todos os ouvintes.

## Parâmetros LoRa

Devem bater com o `receiver-lora` e com o satellite (flight-computer):

| Parâmetro       | Valor    |
|-----------------|----------|
| Frequência      | 915 MHz  |
| Sync Word       | 0xF3     |
| Spreading Factor| 7        |
| Bandwidth       | 125 kHz  |
| Coding Rate     | 4/5      |
| TX Power        | 17 dBm   |

## Configurações (config.h)

| Parâmetro                 | Default | Descrição                                      |
|---------------------------|---------|------------------------------------------------|
| `BEACON_ID`               | "B1"    | Gravar ID único por beacon ("B1".."B8")        |
| `SAT_TEAM_ID`             | "#213"  | Mission ID do satellite escutado               |
| `REPORT_MIN_INTERVAL_MS`  | 250     | Intervalo mínimo entre reports (duty cycle)    |
| `HEARTBEAT_INTERVAL_MS`   | 2000    | Report de status quando não ouve o satellite    |
| `TX_JITTER_MIN/MAX_MS`    | 15/120  | Jitter anti-colisão                             |

## Build e Upload

```bash
cd components/beacon-lora/firmware

pio run                      # build
pio run -t upload            # upload
pio device monitor -b 115200 # monitor serial
```

**Importante**: cada beacon do mesh deve ter `BEACON_ID` distinto — ajuste
`config.h` antes de gravar cada unidade.