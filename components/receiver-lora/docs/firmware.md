# Receiver LoRa — Firmware

## Visao geral

Receiver LoRa do sistema de recuperacao Serra Rocketry. Recebe pacotes de
telemetria via radio LoRa (915 MHz) do satellite, preenche campos de hora/data
com GPS local, e retransmite via Serial (USB) no formato do protocolo Recovery
WebUI. Também é o **nó central do mesh de trilateração**: coleta RSSI de
beacons vizinhos que escutam o mesmo satellite e calcula uma lat/lon corrigida
quando o GPS do satellite estiver indisponível/ruidoso.

**Contexto**: O receiver fica no solo e precisa receber dados do satellite em voo
para alimentar o Recovery WebUI em tempo real. Pacotes perdidos e erros de parse
sao contados e logados na Serial para diagnostico.

## Arquitetura

```
Receiver LoRa
├── config.h               — Configuracoes (pinos, LoRa, params de trilateracao)
├── main.cpp               — Setup + loop (RX LoRa, dispatch, cesto, emissao)
├── LoraReceiver.h/cpp     — Driver LoRa (modo RX continuo + TX para debug)
├── GpsModule.h/cpp        — Wrapper TinyGPSPlus (GPS local do receiver)
├── payload.h              — Formatacao dos pacotes CSV (protocolo)
└── Trilateration.h/cpp    — RSSI->distancia, ajuste de altitude, LSQ 2D
```

### Fluxo de operacao (loop principal)

```
┌──────────────┐     LoRa 915MHz     ┌──────────────┐     USB Serial     ┌──────────────┐
│   Satellite  │ ──────────────────> │   Receiver   │ ─────────────────> │ Recovery     │
│   (#213 TX)  │  pacote 18 campos   │   (RX)       │  pacote v2.0      │ WebUI        │
└──────────────┘                     └──────────────┘                    └──────────────┘
       ▲                                     ▲  │
       │ LoRa                                │  │ report #Bx (11 campos)
       └───────────────  Beacons  ───────────┘  │ (GPS+RSSI do mesmo pkt)
                     (escuta o satellite)        ▼
                                          dispatch: #213 -> cesto/trilateracao
                                                    #11/#51 -> retransmissao direta
                                                    #Bx -> tabela de beacons
```

1. `gpsProcess()` — alimenta parser GPS local
2. `loraAvailable()` / `loraReceive()` — recebe pacote via LoRa
3. `dispatchPacket()` — decide o fluxo pelo TEAM_ID:
   - `#213` (satellite) → cesto de sincronização + trilateração
   - `#11`/`#51` (foguetes) → retransmissão direta (fluxo antigo)
   - `#B1`..`#B8` (reports de beacon) → tabela de beacons + amostra no cesto
4. `processBasket()` — em cada iteração fechar cestos prontos/estourados e emitir
5. `logMeshStatus()` — log periódico: beacons ativos / ouvindo o satellite
6. `Serial.println()` — retransmite para o Recovery WebUI (formato v2.0)

## Mesh de trilateracao

### Conceito

Trilateração por RSSI: cada ouvinte (receiver + beacons) que escuta o MESMO
pacote do satellite mede o RSSI. O modelo log-distance converte RSSI em
distância (slant); a altitude do satellite (do próprio pacote) projeta essa
distância no chão (Pitágoras). Com ≥3 ouvintes de posições conhecidas (GPS de
cada um), um least squares 2D resolve a posição projetada do satellite.

```
        S (satellite, h acima de todos)
       /|\
      / | \        r_i = 10 ^ ((TX_POWER - RSSI_i) / (10*n))   [slant]
     /  |  \       d_i = sqrt(r_i^2 - h^2)                     [projecao no chao]
    /   |   \      A·x = b  (LSQ 2D) -> lat/lon corrigida
   R    B1   B2    R, B1, B2 = ouvintes (GPS proprio, altitude iguais)
```

### Cesto de sincronização

O receiver **não emite na hora** o pacote `#213`: guarda no cesto
(`PendingPacket basket[8]`, índice = `count % 8`) e espera até
`TRILAT_BASKET_WAIT_MS` (900 ms) para coletar os reports `#Bx` que
referenciam o **mesmo `count`** do satellite.

- **Cesto fecha** (`nSamples >= TRILAT_MIN_LISTENERS`): aplica a correção e
  emite já, com a lat/lon corrigida (se o resíduo for aceitável).
- **Prazo estoura**: emite com o GPS cru do pacote do satellite (beste effort).
- **Resíduo alto** (`> TRILAT_MAX_RESIDUAL_M`): emite com o GPS cru — a
  solução LSQ explodiu e o GPS do satellite é mais confiável.

Amostras de beacon entram no cesto apenas se:
- `r.sat_millis != 0` (report referencia um pacote, não heartbeat);
- `r.gpsv == 1` (beacon com fix GPS próprio);
- idade do RSSI no relógio do beacon `<= BEACON_REPORT_MAX_AGE_MS` (1,5 s) —
  garante que todos os ouvintes mediram o MESMO pacote.

### Correção só para o satellite

`TRILAT_MIN_LISTENERS` ouvintes são contados com o receiver incluído. A
correção é aplicada **somente** a `#213` (satellite, sem atuadores de
recuperação própria). Foguetes (`#11`, `#51`) seguem pelo fluxo antigo de
retransmissão direta; seus pacotes também não alimentam o cesto.

### Checagem de beacons

`logMeshStatus()` imprime a cada `MESH_STATUS_INTERVAL_MS`:

```
[MESH] Beacons ativos: 3/3 | ouvindo o satellite: 3 | ultimo pkt: #4521
```

- **Ativo**: report `#Bx` recebido há menos de `BEACON_ACTIVE_WINDOW_MS` (5 s).
- **Ouvindo**: a `sat_millis` do último report é recente (idade ≤
  `BEACON_REPORT_MAX_AGE_MS`) — ou seja, o beacon escutou o satellite há pouco.

## Formato dos Pacotes

### Satellite (#213, Helike) -> Receiver (18 campos, via LoRa)

```
#213,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,alt,lat,lon,sat,rssi#
```

`gp/gr/gy` = giroscopio, `ap/ar/ay` = acelerometro. O satelite **nao envia**
`vz`, `maxAltitude`, `state` e `parachute` (o receiver emite 0 nesses campos
no formato v2.0) — e nem `hora`/`data` (economia de bytes, o receiver
preenche com GPS local). O `#` final marca o fim de pacote RF.

### Foguetes #11/#51 (flight-computer v2.0) -> Receiver (22 campos, via LoRa)

```
TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,maxAltitude,state,alt,lat,lon,sat,parachute,rssi
```

O parser aceita os dois formatos e roteia por TEAM_ID: `#213` → cesto de
trilateração; outro TEAM_ID → retransmissão direta.

### Beacon -> Receiver (report #Bx, 11 campos, via LoRa)

```
#B1,millis,count,sat_count,sat_millis,lat,lon,alt,sat,rssi,gpsv
```

| Campo       | Descrição                                                    |
| ----------- | ------------------------------------------------------------ |
| `sat_count` | count do último pacote do satellite ouvido (0 = heartbeat)   |
| `sat_millis`| uptime do beacon quando ouviu esse pacote (para idade do RSSI)|
| `lat/lon/alt`| posição GPS do beacon                                        |
| `rssi`      | RSSI do pacote do satellite ouvido (-127 no heartbeat)       |
| `gpsv`      | 1 = fix GPS válido, 0 = sem fix                              |

### Receiver -> Recovery WebUI (formato v2.0, 24 campos, via Serial)

```
TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,vz,maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi
```

Campos `hora`/`data` preenchidos pelo GPS local do receiver; `rssi` é o valor
real medido pelo receiver (`LoRa.packetRssi()`). Quando a trilateração corrige
o `#213`, a lat/lon emitida é a corrigida e o `count` permanece o do pacote.

## Trilateracao — parametros (config.h)

| Parametro               | Default | Descricao                                        |
|-------------------------|---------|--------------------------------------------------|
| `SAT_TEAM_ID`           | `#213`  | Mission ID que recebe correcao                   |
| `TRILAT_MIN_LISTENERS`  | 3       | Ouvintes minimos p/ fechar o cesto               |
| `TRILAT_BASKET_WAIT_MS` | 900     | Janela de coleta de reports (ms)                 |
| `TRILAT_MAX_RESIDUAL_M` | 1500    | Residuo LSQ maximo aceito (m)                    |
| `TRILAT_TX_POWER_DBM`   | 20      | Potencia TX do satellite Helike (dBm)           |
| `TRILAT_PATH_LOSS_N`    | 2.0     | Expoente de perda de percurso                    |
| `TRILAT_ALT_CORRECTION` | 1       | Projeta slant no chao via Pitagoras              |
| `TRILAT_MAX_BEACONS`    | 8       | Beacons rastreados (B1..B8)                      |
| `TRILAT_BASKET_SLOTS`   | 8       | Slots do cesto (`count % 8`)                     |
| `BEACON_ACTIVE_WINDOW_MS` | 5000  | Janela de beacon "ativo"                          |
| `BEACON_REPORT_MAX_AGE_MS` | 1500 | Idade maxima do RSSI referenciado                |

### Precisao esperada (simulacao Monte Carlo, mesmo codigo do firmware)

| Cenario                     | Erro mediano | p95     |
|-----------------------------|--------------|---------|
| Sem ruido                   | 0 m          | 0 m     |
| Ruido RSSI ±1 dB            | 134 m        | 309 m   |
| Ruido RSSI ±2 dB            | 268 m        | 744 m   |
| Ruido RSSI ±2 dB + filtro residuo 1500 m | 267 m | 733 m (0/4000 > 3 km) |

Interpretação: a correção é um **refinamento grosseiro** (cross-check), não um
fix primário — com ±2 dB o GPS cru do satellite (tipicamente ±5-10 m) continua
sendo a fonte primária. A trilateração brilha quando o GPS do satellite falha
ou diverge, mantendo uma estimativa plausível (~150-300 m de raio). O filtro
de resíduo elimina completamente as soluções que "explodem" (verificado em
4000 trials).

## Comunicacao LoRa

| Parametro       | Valor     | Nota                           |
|-----------------|-----------|--------------------------------|
| Frequencia      | 915 MHz   | Americas/Brasil                |
| Sync Word       | 0xF3      | Deve bater com satellite       |
| Spreading Factor| 7         | SF7                            |
| Bandwidth       | 125 kHz   |                                |
| Coding Rate     | 4/5       |                                |
| TX Power        | 17 dBm    |                                |
| CRC             | habilitado|                                |

## Contagem de Pacotes Perdidos

O receiver rastreia o campo `count` do satellite para detectar pacotes perdidos:

- **Sem perda**: silencioso (nao polui a Serial)
- **Salto no count**: loga `[LOST] N pacote(s) perdido(s) — count X -> Y | total perdidos: Z`
- **Reset de count** (reboot do satellite): loga `[STATS] Count resetado — X -> Y`
- **Erro de parse**: loga `[STATS] Erro de parse #N` + estatisticas acumuladas
- **Primeiro pacote**: loga `[STATS] Primeiro pacote recebido — count=N`

Exemplo de log na Serial:

```
[LoRa] RX (RSSI=-67): #213,1205,42,150.50,25.30,45.20,1013.25,0.50,1.20,-0.30,0.10,0.20,9.80,150.00,-23.550500,-46.633300,8,0,-1
[LOST] 3 pacote(s) perdido(s) — count 45 -> 49 | total perdidos: 3
[STATS] Recebidos: 48 | Perdidos: 3 | Erros parse: 0 | Taxa perda: 6%
[MESH] Beacons ativos: 3/3 | ouvindo o satellite: 3 | ultimo pkt: #4521
[TRI] pkt#4523 corrigido: -21.775551,-48.175565 | ouvintes=4 | residuo=112.3m
```

## Pinagem

| Pino | Funcao     | Componente       |
|------|------------|------------------|
| 2    | SPI MISO   | RFM95W MISO      |
| 3    | SPI MOSI   | RFM95W MOSI      |
| 4    | SPI SCK    | RFM95W SCK       |
| 5    | SPI CS     | RFM95W chip sel  |
| 6    | LoRa RST   | RFM95W reset     |
| 7    | LoRa IRQ   | RFM95W DIO0      |
| 20   | UART1 RX   | GPS NEO-8M TX    |
| 21   | UART1 TX   | GPS NEO-8M RX    |

## Build e Upload

```bash
cd components/receiver-lora/firmware

# Build
pio run

# Upload
pio run -t upload --upload-port /dev/ttyACM0

# Serial monitor
pio device monitor -b 115200
```

## Antena

### Receiver: Yagi 3-5 elementos (915 MHz)

- Ganho: 7-10 dBi
- Polarizacao: vertical
- Beamwidth: 40-60o horizontal, 30-50o vertical
- Boom: ~13-26 cm
- Apontar na direcao do satellite (azimute) com elevacao de ~0-35o

### Satellite: Monopolo lambda/4 de fita metrica

- Comprimento: ~8 cm (lambda/4 em 915 MHz)
- Ganho: 2-5 dBi
- Polarizacao: vertical
- Montado no corpo do PocketQube, apontando para baixo
- Corpo metalico como counterpoise

### Link Budget estimado (1.5 km)

| Cenario              | Distancia max |
|----------------------|---------------|
| Free space           | 147 km        |
| Rural / vegetacao    | 21 km         |
| Suburban             | 6 km          |
| Suburban denso       | 2.4 km        |

## ADRs

### ADR-001: Receiver em modo RX continuo

**Status**: Aceito

**Contexto**: O receiver precisa receber pacotes do satellite a 5Hz sem perda.

**Decisao**: Usar `LoRa.receive()` modo continuo (nao polling com `parsePacket`). O
ISR do DIO0 acorda o loop para processar.

**Consequencias**:
- (+) Latencia minima entre recebimento e processamento
- (-) Consumo de energia maior (recepcao continua)
- (-) Impossivel transmitir enquanto recebe (nao e necessario)

### ADR-002: Hora/data preenchidos pelo receiver

**Status**: Aceito

**Contexto**: Transmitir hora/data do GPS do satellite via LoRa adiciona 2 campos
(~20 bytes) ao pacote.

**Decisao**: Omitir hora/data do pacote LoRa. O receiver preenche com dados do
seu GPS local. Se nao tiver fix, fica 0.

**Consequencias**:
- (+) Pacote ~20 bytes menor
- (+) Complexidade do satellite reduzida
- (-) Hora/data reflete o receiver, nao o satellite
- (-) Se receiver nao tem GPS fix, campos ficam 0

Na pratica, o `app.py` ja adiciona `NOW` (timestamp local) ao receber, entao a
informacao temporal esta presente independentemente.

### ADR-003: Trilateracao via cesto de sincronizacao

**Status**: Aceito

**Contexto**: Um único ouvinte só tem RSSI próprio — precisa da amostra de
outros ouvintes sobre o MESMO pacote para triangular. Beacons reportam por
jitter aleatório, então o receiver precisa esperar.

**Decisao**: Cesto indexado por `count % 8` com janela de 900 ms. O pacote
`#213` só é emitiado quando o cesto fecha (≥3 ouvintes) ou o prazo estoura
(emissão com GPS cru). Amostra de beacon é validada por idade do RSSI no
relógio do beacon (≤ 1,5 s) e fix GPS próprio.

**Consequencias**:
- (+) Correção só quando ≥3 ouvintes escutaram o mesmo pacote
- (+) Fallback determinístico para GPS cru no estouro do prazo
- (+) Filtro de resíduo (1500 m) elimina soluções LSQ que explodiram
- (-) Latência de até 900 ms na emissão do `#213` corrigido
- (-) Precisão limitada pelo ruído do RSSI (~150-300 m com ±2 dB) — refinamento, não fix primário