# Arquitetura do Sistema

Este documento detalha a arquitetura técnica do Recovery WebUI, explicando como os diferentes componentes interagem.

## Índice

- [Visão Geral](#visão-geral)
- [Arquitetura de Alto Nível](#arquitetura-de-alto-nível)
- [Componentes Backend](#componentes-backend)
- [Componentes Frontend](#componentes-frontend)
- [Fluxo de Dados](#fluxo-de-dados)
- [Comunicação Serial](#comunicação-serial)
- [Sistema de Logs](#sistema-de-logs)
- [Decisões de Design](#decisões-de-design)

## Visão Geral

O Recovery WebUI segue uma arquitetura cliente-servidor com comunicação em tempo real via WebSocket. O sistema é dividido em três camadas principais:

1. **Camada de Hardware**: Módulo LoRa receptor conectado via USB
2. **Camada de Backend**: Aplicação Flask com comunicação serial e WebSocket
3. **Camada de Frontend**: Interface web com mapa interativo

```
┌─────────────────┐
│  Módulo LoRa    │ (Hardware)
│    Receptor     │
└────────┬────────┘
         │ USB/Serial
         ▼
┌─────────────────┐
│  SerialCOM.py   │ (Backend - Comunicação)
│  (PySerial)     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    app.py       │ (Backend - Servidor)
│ Flask + SocketIO│
└────────┬────────┘
         │ WebSocket
         ▼
┌─────────────────┐
│   app.js +      │ (Frontend - Cliente)
│  Leaflet.js     │
└─────────────────┘
```

## Arquitetura de Alto Nível

### Camada de Hardware

- **Módulo LoRa Receptor**: Recebe transmissões dos dispositivos remotos (foguete e satélite)
- **Interface Serial**: Comunicação USB (tipicamente `/dev/ttyACM0` ou `/dev/ttyUSB0`)
- **Protocolo**: Dados em formato CSV via UART

### Camada de Backend

#### Flask Application (`app.py`)

- **Função**: Servidor web e gerenciador de comunicação
- **Responsabilidades**:
  - Servir páginas HTML
  - Gerenciar conexões WebSocket
  - Coordenar comunicação serial
  - Registrar dados em CSV

#### SerialCOM Module (`src/modules/SerialCOM.py`)

- **Função**: Abstração da comunicação serial
- **Responsabilidades**:
  - Inicializar porta serial
  - Ler dados continuamente
  - Gerenciar erros de comunicação
  - Detectar portas disponíveis

#### Flask-SocketIO

- **Função**: Comunicação bidirecional em tempo real
- **Eventos**:
  - `connect`: Cliente conecta ao servidor
  - `disconnect`: Cliente desconecta
  - `updateRocket`: Envia dados do foguete
  - `updateSat`: Envia dados do satélite

### Camada de Frontend

#### Templates HTML

- **base.html**: Template base com estrutura comum
- **index.html**: Página de rastreamento do foguete
- **satellite.html**: Página de rastreamento do satélite

#### JavaScript (`app.js`)

- **Socket.IO Client**: Recebe atualizações em tempo real
- **Leaflet.js**: Renderiza mapas interativos
- **jQuery**: Manipulação do DOM e requisições AJAX

#### CSS

- **app.css**: Estilos customizados
- **FontAwesome**: Ícones
- **Leaflet.css**: Estilos do mapa

## Componentes Backend

### app.py - Detalhamento

```python
# Estrutura Principal
┌────────────────────────────┐
│   Flask Application        │
│                            │
│  ┌──────────────────────┐  │
│  │ Routes               │  │
│  │ - / (index)          │  │
│  │ - /satellite         │  │
│  └──────────────────────┘  │
│                            │
│  ┌──────────────────────┐  │
│  │ SocketIO Events      │  │
│  │ - connect            │  │
│  │ - disconnect         │  │
│  └──────────────────────┘  │
│                            │
│  ┌──────────────────────┐  │
│  │ Background Thread    │  │
│  │ - Lê porta serial    │  │
│  │ - Processa dados     │  │
│  │ - Emite WebSocket    │  │
│  │ - Salva em CSV       │  │
│  └──────────────────────┘  │
└────────────────────────────┘
```

#### Background Thread

A thread de background é responsável pelo loop principal de leitura:

```python
def background_thread():
    # 1. Cria arquivo de log CSV
    # 2. Loop infinito:
    #    - Lê dados da serial
    #    - Parse do CSV
    #    - Identifica TEAM_ID
    #    - Emite evento WebSocket apropriado
    #    - Salva em log
    #    - Sleep breve (0.5s)
```

**Características**:

- Executa em thread separada (não bloqueia o servidor)
- Trata exceções para evitar crashes
- Usa `socketio.sleep()` para cooperação com eventloop
- Cria arquivo de log na inicialização

### SerialCOM.py - Detalhamento

```python
class BaseCom:
    ├── __init__()           # Inicializa (port=None, lazy)
    ├── open()               # Abre conexao serial
    ├── close()              # Fecha conexao
    ├── send_command()       # Envia comandos (nao usado no momento)
    ├── read_response()      # Le uma linha da serial
    ├── check_connected()    # Verifica se porta esta aberta
    ├── get_port() / set_port()
    ├── get_baudrate() / set_baudrate()
    └── get_port_options() / get_baudrate_options()
```

def list_ports():           # Lista portas seriais disponíveis
    ├── Windows: Todas as portas COM
    ├── Linux: Apenas /dev/ttyACM*
    └── macOS: Todas as portas
```

**Configuração Serial**:

- **Baudrate**: 115200 (padrão LoRa)
- **Timeout**: 0.5s
- **Flow Control**: Desabilitado (xonxoff, rtscts, dsrdtr)
- **Encoding**: UTF-8

## Componentes Frontend

### app.js - Detalhamento

```javascript
┌─────────────────────────────┐
│  Inicialização do Mapa      │
│  - Leaflet.js               │
│  - Camadas base             │
│  - Layer groups             │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Socket.IO Client           │
│  - Conecta ao servidor      │
│  - Escuta eventos           │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Event Handlers             │
│  - updateRocket             │
│  - updateSat                │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Atualização de UI          │
│  - addData() / addSatData() │
│  - MapPoint() / MapSatPoint()│
│  - Atualiza tabela HTML     │
└─────────────────────────────┘
```

#### Estrutura de Camadas do Mapa

O mapa usa três camadas base alternáveis:

- **Google Satélite**: Imagens de satélite
- **Google Streets**: Mapa de ruas
- **OpenStreetMap**: Mapa colaborativo

E duas camadas de overlay:

- **Foguete**: Marcadores do foguete (#100)
- **Satélite**: Marcadores do satélite (#261)

#### Fluxo de Atualização

```javascript
WebSocket Event
    ↓
Parse JSON data
    ↓
Identificar tipo (Rocket/Sat)
    ↓
┌─────────────────┬──────────────────┐
│ Rocket          │ Satellite        │
├─────────────────┼──────────────────┤
│ addData()       │ addSatData()     │
│   ├─ MapPoint() │   ├─ MapSatPoint()│
│   └─ Add to     │   └─ Add to      │
│      table      │      satTable    │
└─────────────────┴──────────────────┘
```

## Fluxo de Dados

### 1. Inicialização do Sistema

```
Usuario executa app.py
    ↓
Lista portas seriais disponíveis
    ↓
Usuario seleciona porta
    ↓
Cria objeto BaseCom(porta)
    ↓
Inicia servidor Flask na porta 5000
    ↓
Abre navegador automaticamente
```

### 2. Conexão do Cliente

```
Cliente acessa http://localhost:5000
    ↓
Servidor serve index.html
    ↓
Cliente carrega app.js
    ↓
app.js estabelece conexão WebSocket
    ↓
Servidor detecta evento 'connect'
    ↓
Servidor inicia background_thread
```

### 3. Recepção e Transmissão de Dados

```
Módulo LoRa recebe transmissão
    ↓
Envia via USB para computador
    ↓
SerialCOM.read_response() lê linha
    ↓
background_thread processa dados
    ↓
Parse CSV e identifica TEAM_ID
    ↓
┌────────────────────┬─────────────────┐
│ #100 (Foguete)     │ #261 (Satélite) │
├────────────────────┼─────────────────┤
│ socketio.emit      │ socketio.emit   │
│ ('updateRocket')   │ ('updateSat')   │
└──────────┬─────────┴────────┬────────┘
           │                  │
           ▼                  ▼
    Cliente recebe       Cliente recebe
           │                  │
           ▼                  ▼
    addData()            addSatData()
           │                  │
           ▼                  ▼
    Atualiza mapa        Atualiza mapa
    e tabela             e tabela
```

### 4. Persistência de Dados

```
Dados recebidos na serial
    ↓
background_thread processa
    ↓
Adiciona timestamp NOW
    ↓
Escreve em web/static/logs/log.csv
    ↓
Formato: NOW,TEAM_ID,millis,count,...
```

## Comunicação Serial

### Formato dos Dados

```csv
TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,hora,data,alt,lat,lon,sat,rssi
#100,12345,42,150.5,25.3,45.2,1013.25,0.5,1.2,-0.3,0.1,0.2,9.8,123045,20250120,150.0,-23.5500,-46.6333,8,0,-75
```

### Estrutura da Classe Serial

```python
BaseCom(port, baudrate=115200, timeout=0.5)
    │
    ├─ Configuração
    │   ├─ xonxoff = False      (sem controle XON/XOFF)
    │   ├─ rtscts = False        (sem controle RTS/CTS)
    │   ├─ dsrdtr = False        (sem controle DSR/DTR)
    │   └─ inter_byte_timeout    (sem timeout entre bytes)
    │
    └─ Métodos
        ├─ read_response()       (readline + decode UTF-8)
        ├─ send_command()        (write bytes)
        ├─ check_connection()    (is_open)
        └─ close()              (fecha porta)
```

## Sistema de Logs

### Estrutura do Arquivo CSV

```
Header:
NOW,TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,hora,data,alt,lat,lon,sat,rssi

Linha de exemplo:
2025-01-20 14:30:45,#100,12345,42,150.5,25.3,45.2,1013.25,0.5,1.2,-0.3,0.1,0.2,9.8,143045,20250120,150.0,-23.5500,-46.6333,8,0,-75
```

### Localização

- **Desenvolvimento**: `web/static/logs/log.csv`
- **Criado em**: Inicialização da background_thread
- **Modo**: Append ('a') para cada nova entrada

## Decisões de Design

### Por que Flask + SocketIO?

- **Simplicidade**: Flask é leve e fácil de entender
- **Tempo Real**: SocketIO fornece comunicação bidirecional
- **Python**: Facilita integração com PySerial
- **Multiplataforma**: Funciona em Windows, Linux e macOS

### Por que Leaflet.js?

- **Open Source**: Sem custos ou limitações de API
- **Leve**: ~40KB minificado
- **Flexível**: Suporta múltiplas fontes de tiles
- **Bem documentado**: Grande comunidade

### Por que Background Thread?

- **Não Bloqueante**: Servidor Flask continua respondendo
- **Isolamento**: Erros na serial não derrubam o servidor
- **Performance**: Leitura contínua sem polling HTTP

### Por que CSV para Logs?

- **Simplicidade**: Fácil de ler e processar
- **Compatibilidade**: Abrível em Excel, Python, R, etc.
- **Performance**: Escrita rápida sem overhead de banco de dados
- **Portabilidade**: Arquivo texto simples

### Limitações Conhecidas

1. **Escalabilidade**: Background thread única pode ser gargalo com múltiplos dispositivos
2. **Persistência**: Sem banco de dados relacional
3. **Autenticação**: Sem autenticação/autorização
4. **Validação**: Validação limitada dos dados recebidos
5. **Recuperação**: Sem mecanismo de reconexão automática da serial

### Melhorias Futuras Possíveis

1. **Banco de Dados**: SQLite ou PostgreSQL para queries complexas
2. **Múltiplos Dispositivos**: Thread pool ou async/await
3. **API REST**: Endpoints para consulta de histórico
4. **Autenticação**: Login para acesso remoto seguro
5. **Cache**: Redis para dados em tempo real
6. **Logging Estruturado**: Sistema de logging mais robusto

## Diagramas de Sequência

### Sequência de Inicialização

```
Usuario     app.py      SerialCOM    SocketIO    Browser
  │           │             │           │           │
  │──run──────>│             │           │           │
  │           │──list────>  │           │           │
  │           │<──ports────  │           │           │
  │<──prompt──│             │           │           │
  │──select───>│             │           │           │
  │           │──init─────>  │           │           │
  │           │<──OK────────  │           │           │
  │           │──start─────────────────> │           │
  │           │──open_browser──────────────────────> │
  │           │                           │<──GET────│
  │           │<──────────────────────────│          │
  │           │──HTML─────────────────────────────>  │
  │           │                           │<──WS─────│
  │           │<──connect─────────────────│          │
  │           │──start_thread──>          │          │
```

### Sequência de Recepção de Dados

```
LoRa      SerialCOM    background_thread    SocketIO    Browser
  │           │              │                 │           │
  │──data───> │              │                 │           │
  │           │<──read───────│                 │           │
  │           │──line──────> │                 │           │
  │           │              │──parse──>       │           │
  │           │              │──emit────────>  │           │
  │           │              │                 │──JSON────>│
  │           │              │──write_log──>   │           │
```

## Considerações de Performance

### Latência

- **Serial Read**: ~0.5s (timeout configurado)
- **WebSocket**: <50ms (típico em localhost)
- **Mapa Render**: ~100ms (depende do navegador)
- **Latência Total**: ~1s do transmissor até visualização

### Throughput

- **Taxa Serial**: 115200 baud = ~11.5 KB/s teórico
- **Pacote típico**: ~150 bytes = ~77 pacotes/s máximo
- **Taxa real**: Limitada pelo sleep(0.5) = 2 pacotes/s

### Uso de Recursos

- **Memória**: ~50-100 MB (Python + Flask + SocketIO)
- **CPU**: <5% (single thread, I/O bound)
- **Rede**: ~1-2 KB/s (dados de telemetria)
- **Disco**: ~50 KB/hora de log

## Segurança

### Considerações Atuais

- **Sem autenticação**: Qualquer cliente pode conectar
- **Localhost apenas**: Servidor escuta em 0.0.0.0 mas destinado para uso local
- **Sem criptografia**: Dados transmitidos em texto plano
- **Sem validação**: Dados da serial não são validados rigorosamente

### Para Uso em Produção

Se for expor o sistema na internet:

1. Adicionar autenticação (Flask-Login)
2. Usar HTTPS (certificados SSL/TLS)
3. Validar e sanitizar dados de entrada
4. Implementar rate limiting
5. Usar firewall e restrições de rede

---

[← Voltar ao README](../README.md) | [Próximo: API →](api.md)
