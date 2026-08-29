# Recovery WebUI

Sistema de rastreamento e recuperação em tempo real para foguetes experimentais, desenvolvido para monitorar telemetria via comunicação serial LoRa e visualizar dados em interface web.

## 📋 Visão Geral

O Recovery WebUI é uma aplicação Flask que fornece uma interface web para monitoramento em tempo real de foguetes experimentais. O sistema recebe dados de telemetria via comunicação serial (tipicamente através de módulos LoRa), processa as informações e as exibe em um mapa interativo, permitindo o rastreamento da posição, altitude, e outros parâmetros importantes durante o voo.

### Funcionalidades Principais

- 🗺️ **Visualização em Mapa Interativo**: Mapa com múltiplas camadas (Google Satélite, Google Streets, OpenStreetMap)
- 📡 **Rastreamento em Tempo Real**: Comunicação via WebSocket para atualizações instantâneas
- 📊 **Registro de Dados**: Armazenamento automático de telemetria em formato CSV
- 🚀 **Suporte Dual**: Rastreamento simultâneo de foguete (#100) e satélite (#261)
- 📈 **Histórico de Voo**: Tabelas com histórico completo de dados recebidos
- 🔌 **Comunicação Serial**: Integração com dispositivos via porta serial (LoRa)

### Dados Monitorados

#### Foguete (TEAM_ID #100)

- Posição GPS (latitude, longitude)
- Altitude barométrica
- Número de satélites GPS
- Status do paraquedas
- RSSI (intensidade do sinal)

#### Satélite (TEAM_ID #261)

- Posição GPS (latitude, longitude)
- Altitude
- Temperatura
- Umidade relativa
- Pressão atmosférica
- Número de satélites GPS
- RSSI (intensidade do sinal)

## 🚀 Início Rápido

### Pré-requisitos

- Python 3.7 ou superior
- Porta serial disponível (para comunicação com o módulo LoRa)
- Navegador web moderno

### Instalação

1. Clone o repositório:

```bash
git clone <url-do-repositorio>
cd recovery-webui
```

2. Instale as dependências:

```bash
pip install -r requirements.txt
```

### Uso

1. Conecte o módulo LoRa receptor ao computador via USB

2. Execute a aplicação:

```bash
cd app
python app.py
```

3. Selecione a porta serial quando solicitado:

```
Portas seriais disponíveis:
1: /dev/ttyACM0
2: /dev/ttyUSB0
Selecione a porta serial (número): 1
```

4. O navegador abrirá automaticamente em `http://localhost:5000`

5. Acesse as diferentes visualizações:
   - `/` - Dados do foguete principal
   - `/satellite` - Dados do satélite

## 📁 Estrutura do Projeto

```
recovery-webui/
├── app/
│   ├── app.py              # Aplicação Flask principal
│   ├── module/
│   │   ├── __init__.py
│   │   └── SerialCOM.py    # Módulo de comunicação serial
│   ├── static/
│   │   ├── css/            # Estilos
│   │   └── js/
│   │       └── app.js      # Lógica do cliente (WebSocket, mapa)
│   ├── templates/
│   │   ├── base.html       # Template base
│   │   ├── index.html      # Página do foguete
│   │   └── satellite.html  # Página do satélite
│   └── logs/
│       └── log.csv         # Logs de telemetria
├── requirements.txt        # Dependências Python
└── README.md              # Este arquivo
```

## 🛠️ Tecnologias Utilizadas

### Backend

- **Flask** - Framework web Python
- **Flask-SocketIO** - Comunicação WebSocket em tempo real
- **PySerial** - Comunicação serial com hardware

### Frontend

- **Leaflet.js** - Biblioteca de mapas interativos
- **Socket.IO** - Cliente WebSocket
- **jQuery** - Manipulação do DOM
- **Font Awesome** - Ícones

## 📡 Protocolo de Comunicação

O sistema espera dados no seguinte formato CSV via serial:

```
TEAM_ID,millis,count,altp,temp,umi,p,gp,gr,gy,ap,ar,ay,hora,data,alt,lat,lon,sat,rssi
```

Onde:

- `TEAM_ID`: Identificador do dispositivo (#100 para foguete, #261 para satélite)
- `millis`: Tempo em milissegundos desde o boot
- `count`: Contador de pacotes
- `altp`: Altitude barométrica
- `temp`: Temperatura (°C)
- `umi`: Umidade relativa (%)
- `p`: Pressão atmosférica (hPa)
- `gp`, `gr`, `gy`: Dados do giroscópio (pitch, roll, yaw)
- `ap`, `ar`, `ay`: Dados do acelerômetro (x, y, z)
- `hora`, `data`: Hora e data do GPS
- `alt`: Altitude GPS (m)
- `lat`, `lon`: Coordenadas GPS
- `sat`: Número de satélites GPS

- `rssi`: Intensidade do sinal LoRa

## 📚 Documentação Adicional

Para informações mais detalhadas, consulte a pasta `docs/`:

- [Guia de Instalação](docs/installation.md) - Instalação detalhada e configuração
- [Arquitetura do Sistema](docs/architecture.md) - Detalhes técnicos da arquitetura
- [Documentação da API](docs/api.md) - Endpoints e eventos WebSocket
- [Guia de Desenvolvimento](docs/development.md) - Contribuindo para o projeto

## 🐛 Solução de Problemas

### Porta serial não encontrada

- Verifique se o dispositivo está conectado
- Em Linux, você pode precisar de permissões: `sudo usermod -a -G dialout $USER`
- Verifique se o driver CH340/CP2102 está instalado (dependendo do módulo)

### Dados não aparecem no mapa

- Verifique se o formato dos dados serial está correto
- Confira o console do navegador (F12) para erros JavaScript
- Verifique os logs do servidor no terminal

### Navegador não abre automaticamente

- Abra manualmente: `http://localhost:5000`
- Verifique se a porta 5000 não está em uso

## 🤝 Contribuindo

Contribuições são bem-vindas! Por favor:

1. Faça um fork do projeto
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona MinhaFeature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

Leia nosso [Guia de Contribuição](CONTRIBUTING.md) para mais detalhes sobre padrões de código, processo de desenvolvimento e como reportar bugs.

## 📝 Licença

Este projeto é open source e está disponível para uso educacional e experimental.

## 🔗 Projetos Relacionados

- [flight-computer](../flight-computer) - Firmware do computador de bordo do foguete

## 📧 Contato

Para dúvidas ou sugestões, abra uma issue no repositório.

---

Desenvolvido para competições e experimentos de foguetemodelismo 🚀
