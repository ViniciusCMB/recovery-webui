# Documentação da API

Este documento detalha todos os endpoints HTTP e eventos WebSocket disponíveis no Recovery WebUI.

## Índice

- [HTTP Endpoints](#http-endpoints)
- [WebSocket Events](#websocket-events)
- [Formato de Dados](#formato-de-dados)
- [Exemplos de Uso](#exemplos-de-uso)
- [Códigos de Erro](#códigos-de-erro)

## HTTP Endpoints

### GET /

Renderiza a página principal de rastreamento do foguete.

**Request:**

```http
GET / HTTP/1.1
Host: localhost:5000
```

**Response:**

```http
HTTP/1.1 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
...
</html>
```

**Descrição:**

- Página com mapa interativo
- Tabela de dados do foguete (TEAM_ID #100)
- Atualização em tempo real via WebSocket

---

### GET /satellite

Renderiza a página de rastreamento do satélite.

**Request:**

```http
GET /satellite HTTP/1.1
Host: localhost:5000
```

**Response:**

```http
HTTP/1.1 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
...
</html>
```

**Descrição:**

- Página com mapa interativo
- Tabela de dados do satélite (TEAM_ID #261)
- Dados adicionais: temperatura, umidade, pressão
- Atualização em tempo real via WebSocket

---

## WebSocket Events

O sistema usa Socket.IO para comunicação em tempo real entre servidor e clientes.

### Cliente → Servidor

#### connect

Evento emitido automaticamente quando o cliente estabelece conexão.

**Trigger:** Automático ao carregar a página

**Ação do Servidor:**

- Registra cliente conectado
- Inicia background thread (se ainda não iniciada)

**Exemplo (JavaScript):**

```javascript
var socket = io.connect();

socket.on("connect", function () {
  console.log("Conectado ao servidor");
});
```

---

#### disconnect

Evento emitido automaticamente quando o cliente desconecta.

**Trigger:** Automático ao fechar a página/tab

**Ação do Servidor:**

- Registra cliente desconectado
- Log no console do servidor

**Exemplo (JavaScript):**

```javascript
socket.on("disconnect", function () {
  console.log("Desconectado do servidor");
});
```

---

### Servidor → Cliente

#### updateRocket

Evento emitido quando novos dados do foguete são recebidos (TEAM_ID #100).

**Payload:**

```javascript
{
    "latitude": "-23.5505",      // string - Latitude em graus decimais
    "longitude": "-46.6333",     // string - Longitude em graus decimais
    "altura": "150.5",           // string - Altitude barométrica em metros
    "satelites": "8",            // string - Número de satélites GPS
    "rssi": "-75",               // string - RSSI do sinal LoRa em dBm

    "time": "2025-01-20 14:30:45" // string - Timestamp servidor
}
```

**Exemplo (JavaScript):**

```javascript
socket.on("updateRocket", function (data) {
  console.log("Dados do foguete:", data);
  var lat = parseFloat(data.latitude);
  var lon = parseFloat(data.longitude);
  var alt = parseFloat(data.altura);

  // Atualizar mapa

});
```

**Frequência:** Aproximadamente a cada 0.5 segundos quando há dados disponíveis

---

#### updateSat

Evento emitido quando novos dados do satélite são recebidos (TEAM_ID #261).

**Payload:**

```javascript
{
    "latitude": "-23.5505",      // string - Latitude em graus decimais
    "longitude": "-46.6333",     // string - Longitude em graus decimais
    "altura": "150.5",           // string - Altitude em metros
    "satelites": "8",            // string - Número de satélites GPS
    "temperatura": "25.3",       // string - Temperatura em °C
    "umidade": "45.2",           // string - Umidade relativa em %
    "pressao": "1013.25",        // string - Pressão atmosférica em hPa
    "rssi": "-75",               // string - RSSI do sinal LoRa em dBm
    "time": "2025-01-20 14:30:45" // string - Timestamp servidor
}
```

**Exemplo (JavaScript):**

```javascript
socket.on("updateSat", function (data) {
  console.log("Dados do satélite:", data);
  var lat = parseFloat(data.latitude);
  var lon = parseFloat(data.longitude);
  var alt = parseFloat(data.altura);
  var temp = parseFloat(data.temperatura);
  var umid = parseFloat(data.umidade);
  var press = parseFloat(data.pressao);

  // Atualizar interface
  addSatData(
    lat,
    lon,
    alt,
    temp,
    umid,
    press,
    data.satelites,
    data.time,
    data.rssi,
  );
});
```

**Frequência:** Aproximadamente a cada 0.5 segundos quando há dados disponíveis

---

## Formato de Dados

### Dados Seriais (CSV)

Formato recebido pela porta serial:

```csv
TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,hora,data,alt,lat,lon,sat,rssi
```

**Campos:**

| Campo   | Tipo   | Descrição                    | Unidade | Exemplo    |
| ------- | ------ | ---------------------------- | ------- | ---------- |
| TEAM_ID | string | Identificador do dispositivo | -       | #100, #261 |
| millis  | int    | Tempo desde boot             | ms      | 12345      |
| count   | int    | Contador de pacotes          | -       | 42         |
| altp    | float  | Altitude barométrica         | m       | 150.5      |
| temp    | float  | Temperatura                  | °C      | 25.3       |
| umi     | float  | Umidade relativa             | %       | 45.2       |
| p       | float  | Pressão atmosférica          | hPa     | 1013.25    |
| gp      | float  | Giroscópio pitch             | °/s     | 0.5        |
| gr      | float  | Giroscópio roll              | °/s     | 1.2        |
| gy      | float  | Giroscópio yaw               | °/s     | -0.3       |
| ap      | float  | Acelerômetro X               | m/s²    | 0.1        |
| ar      | float  | Acelerômetro Y               | m/s²    | 0.2        |
| ay      | float  | Acelerômetro Z               | m/s²    | 9.8        |
| hora    | int    | Hora GPS (HHMMSS)            | -       | 143045     |
| data    | int    | Data GPS (DDMMYYYY)          | -       | 20012025   |
| alt     | float  | Altitude GPS                 | m       | 150.0      |
| lat     | float  | Latitude                     | °       | -23.5505   |
| lon     | float  | Longitude                    | °       | -46.6333   |
| sat     | int    | Número de satélites          | -       | 8          |

| rssi    | int    | RSSI LoRa                    | dBm     | -75        |

**Exemplo completo:**

```
#100,12345,42,150.5,25.3,45.2,1013.25,0.5,1.2,-0.3,0.1,0.2,9.8,143045,20012025,150.0,-23.5505,-46.6333,8,0,-75
```

### Arquivo de Log (CSV)

Formato salvo em `web/static/logs/log.csv`:

```csv
NOW,TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,hora,data,alt,lat,lon,sat,rssi
2025-01-20 14:30:45,#100,12345,42,150.5,25.3,45.2,1013.25,0.5,1.2,-0.3,0.1,0.2,9.8,143045,20012025,150.0,-23.5505,-46.6333,8,0,-75
```

**Campo adicional:**

- **NOW**: Timestamp do servidor no formato `YYYY-MM-DD HH:MM:SS`

---

## Exemplos de Uso

### Cliente HTML/JavaScript Completo

```html
<!DOCTYPE html>
<html>
  <head>
    <title>Cliente WebSocket</title>
    <script src="https://cdn.socket.io/4.5.4/socket.io.min.js"></script>
  </head>
  <body>
    <h1>Rastreamento em Tempo Real</h1>
    <div id="status">Desconectado</div>
    <div id="data"></div>

    <script>
      // Conectar ao servidor
      var socket = io.connect("http://localhost:5000");

      // Evento de conexão
      socket.on("connect", function () {
        document.getElementById("status").textContent = "Conectado";
        console.log("Conectado ao servidor WebSocket");
      });

      // Evento de desconexão
      socket.on("disconnect", function () {
        document.getElementById("status").textContent = "Desconectado";
        console.log("Desconectado do servidor WebSocket");
      });

      // Receber dados do foguete
      socket.on("updateRocket", function (data) {
        console.log("Foguete:", data);

        var html = `
                <h2>Foguete (#100)</h2>
                <p>Posição: ${data.latitude}, ${data.longitude}</p>
                <p>Altitude: ${data.altura} m</p>
                <p>Satélites: ${data.satelites}</p>

                <p>RSSI: ${data.rssi} dBm</p>
                <p>Hora: ${data.time}</p>
            `;

        document.getElementById("data").innerHTML = html;
      });

      // Receber dados do satélite
      socket.on("updateSat", function (data) {
        console.log("Satélite:", data);

        var html = `
                <h2>Satélite (#261)</h2>
                <p>Posição: ${data.latitude}, ${data.longitude}</p>
                <p>Altitude: ${data.altura} m</p>
                <p>Temperatura: ${data.temperatura} °C</p>
                <p>Umidade: ${data.umidade} %</p>
                <p>Pressão: ${data.pressao} hPa</p>
                <p>Satélites: ${data.satelites}</p>
                <p>RSSI: ${data.rssi} dBm</p>
                <p>Hora: ${data.time}</p>
            `;

        document.getElementById("data").innerHTML = html;
      });
    </script>
  </body>
</html>
```

### Cliente Python

```python
import socketio

# Criar cliente Socket.IO
sio = socketio.Client()

# Event handlers
@sio.event
def connect():
    print('Conectado ao servidor')

@sio.event
def disconnect():
    print('Desconectado do servidor')

@sio.on('updateRocket')
def on_rocket_update(data):
    print(f"Foguete - Lat: {data['latitude']}, Lon: {data['longitude']}, Alt: {data['altura']}m")

@sio.on('updateSat')
def on_sat_update(data):
    print(f"Satélite - Lat: {data['latitude']}, Lon: {data['longitude']}, Temp: {data['temperatura']}°C")

# Conectar ao servidor
try:
    sio.connect('http://localhost:5000')
    sio.wait()
except KeyboardInterrupt:
    sio.disconnect()
```

### Requisitos para cliente Python:

```bash
pip install python-socketio[client]
```

---

## Códigos de Erro

### Erros do Servidor

#### Porta Serial Não Encontrada

**Cenário:** Nenhuma porta serial disponível

**Mensagem:**

```
Nenhuma porta serial encontrada.
```

**Solução:**

- Conecte o dispositivo USB
- Verifique drivers (CH340/CP2102)
- Em Linux: verifique permissões (`sudo usermod -a -G dialout $USER`)

---

#### Erro de Conexão Serial

**Cenário:** Falha ao abrir porta serial

**Exception:** `serial.SerialException`

**Mensagem:**

```
Could not open port /dev/ttyACM0: [Errno 13] Permission denied
```

**Solução:**

- Linux: adicione usuário ao grupo dialout
- Windows: verifique se a porta não está em uso
- Reconecte o dispositivo

---

#### Erro de Leitura Serial

**Cenário:** Erro durante leitura da porta serial

**Log:**

```
Erro em background_thread-> [erro específico]
```

**Ações do servidor:**

- Registra erro no console
- Espera 1 segundo
- Continua tentando ler

---

#### Erro de Parse CSV

**Cenário:** Dados recebidos não estão no formato esperado

**Log:**

```
Erro em background_thread-> not enough values to unpack (expected 21, got X)
```

**Possíveis causas:**

- Dados corrompidos na transmissão
- Formato CSV incorreto do transmissor
- Interferência no sinal LoRa

---

### Erros do Cliente

#### Conexão WebSocket Falhou

**Console do navegador:**

```
WebSocket connection to 'ws://localhost:5000/socket.io/' failed
```

**Possíveis causas:**

- Servidor não está rodando
- Porta incorreta
- Firewall bloqueando conexão

**Solução:**

1. Verifique se o servidor está ativo
2. Confirme a porta (padrão: 5000)
3. Desabilite firewall temporariamente para teste

---

#### Dados não Aparecem no Mapa

**Console do navegador:**

```
Uncaught TypeError: Cannot read property 'latitude' of undefined
```

**Possíveis causas:**

- Payload do WebSocket incompleto
- Erro no parse de JSON
- TEAM_ID não reconhecido

**Solução:**

1. Abra o console do navegador (F12)
2. Verifique a aba "Network" → "WS" para mensagens WebSocket
3. Confirme formato dos dados recebidos

---

## Segurança

### Considerações

⚠️ **Aviso:** O sistema atual não possui autenticação ou criptografia.

**Para uso em rede local:**

- Firewall configurado corretamente
- Acesso restrito à rede

**Para exposição na internet:**

- Implementar autenticação (Flask-Login, JWT)
- Usar HTTPS/WSS (certificados SSL/TLS)
- Validar e sanitizar dados de entrada
- Implementar rate limiting

---

## Performance

### Latência Típica

| Etapa               | Tempo       |
| ------------------- | ----------- |
| Transmissão LoRa    | 100-500ms   |
| Leitura Serial      | <50ms       |
| Processamento       | <10ms       |
| WebSocket           | <50ms       |
| Render no navegador | 50-200ms    |
| **Total**           | **~0.5-1s** |

### Taxa de Atualização

- **Configurada**: 2 atualizações/segundo (sleep de 0.5s)
- **Máxima teórica**: ~77 pacotes/segundo (limite do baudrate)
- **Recomendada**: 1-5 atualizações/segundo para visualização suave

---

## Versionamento

Esta documentação refere-se à versão atual do Recovery WebUI.

**Compatibilidade:**

- Flask 2.2.1
- Flask-SocketIO 5.5.1
- Socket.IO client 4.x

---

## Referências

- [Flask Documentation](https://flask.palletsprojects.com/)
- [Flask-SocketIO Documentation](https://flask-socketio.readthedocs.io/)
- [Socket.IO Client Documentation](https://socket.io/docs/v4/client-api/)
- [PySerial Documentation](https://pyserial.readthedocs.io/)

---

[← Voltar ao README](../README.md) | [Próximo: Desenvolvimento →](development.md)
