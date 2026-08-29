# Perguntas Frequentes (FAQ)

Respostas para as dúvidas mais comuns sobre o Recovery WebUI.

## Índice

- [Geral](#geral)
- [Instalação](#instalação)
- [Uso](#uso)
- [Hardware](#hardware)
- [Conectividade](#conectividade)
- [Performance](#performance)
- [Desenvolvimento](#desenvolvimento)

## Geral

### O que é o Recovery WebUI?

O Recovery WebUI é um sistema de rastreamento em tempo real para foguetes experimentais e satélites. Ele recebe dados de telemetria via comunicação serial (geralmente LoRa), processa as informações e exibe em uma interface web com mapa interativo.

### Para que serve?

- Rastrear posição GPS em tempo real
- Monitorar altitude, velocidade e outros parâmetros
- Facilitar a recuperação de foguetes após o voo
- Analisar dados de voo
- Competições de foguetemodelismo

### Quais são os requisitos mínimos?

- Python 3.7 ou superior
- Porta USB disponível
- Módulo LoRa receptor
- Navegador web moderno
- 512 MB RAM
- 100 MB de espaço em disco

### É open source?

Sim, o projeto é open source e está disponível para uso educacional e experimental.

### Quanto custa?

O software é gratuito. Os custos envolvem apenas o hardware (módulo LoRa, antenas, etc.).

## Instalação

### Como instalo no Linux?

```bash
git clone <url-do-repositorio>
cd recovery-webui
pip3 install -r requirements.txt
cd app
python3 app.py
```

Veja o [Guia de Instalação completo](installation.md#instalação-no-linux).

### Funciona no Windows?

Sim! O sistema é multiplataforma. Veja o [Guia de Instalação para Windows](installation.md#instalação-no-windows).

### Preciso instalar drivers?

Dependendo do módulo USB (CH340, CP2102), você pode precisar de drivers específicos:

- **Linux**: Geralmente já inclusos no kernel
- **Windows**: Baixe do site do fabricante
- **macOS**: Pode precisar de drivers específicos

Veja [Drivers Serial](installation.md#drivers-serial-no-windows).

### "Permission denied" ao acessar porta serial

No Linux, adicione seu usuário ao grupo `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Faça logout e login novamente.

### Erro "Port already in use"

A porta 5000 já está sendo usada. Modifique no `app.py`:

```python
port = 5001  # ou outra porta
socketio.run(app, host='0.0.0.0', port=port)
```

## Uso

### Como inicio o sistema?

```bash
cd app
python3 app.py
```

Selecione a porta serial quando solicitado e o navegador abrirá automaticamente.

### Não tenho módulo LoRa. Posso testar?

Sim! Crie um script de simulação:

```python
# test_simulator.py
import serial
import time

# Crie um par de portas virtuais
# Linux: socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Ou use mock no código (veja Guia de Desenvolvimento)
```

### Como visualizo dados do satélite?

Acesse `http://localhost:5000/satellite` no navegador.

### Onde ficam salvos os dados?

Em `web/static/logs/log.csv` no formato CSV com timestamp.

### Como exporto os dados para análise?

O arquivo `log.csv` pode ser aberto em:

- Excel / LibreOffice Calc
- Python (pandas)
- MATLAB
- R
- Qualquer editor de texto

### Posso acessar de outro computador na rede?

Sim! O servidor escuta em `0.0.0.0`. Acesse via:

```
http://<ip-do-servidor>:5000
```

⚠️ **Atenção:** Não há autenticação. Use apenas em redes confiáveis.

### Como limpo os logs antigos?

Simplesmente delete ou renomeie o arquivo:

```bash
mv web/static/logs/log.csv web/static/logs/log_backup_$(date +%Y%m%d).csv
```

## Hardware

### Qual módulo LoRa devo usar?

Módulos populares compatíveis:

- **SX1276/SX1278**: 433/868/915 MHz
- **Ra-02**: Econômico e eficiente
- **E32-TTL**: Fácil de usar
- **RFM95/96/97/98**: Alta qualidade

### Qual frequência devo usar?

Depende da sua região:

- **433 MHz**: Europa, Ásia (mais comum)
- **868 MHz**: Europa
- **915 MHz**: América do Norte, Austrália

Verifique a legislação local!

### Qual o alcance esperado?

| Condição                     | Alcance   |
| ---------------------------- | --------- |
| Linha de vista, campo aberto | 5-15 km   |
| Área urbana                  | 1-3 km    |
| Dentro de prédios            | 100-500 m |
| Com boas antenas             | Até 30 km |

### Que tipo de antena devo usar?

- **Transmissor (foguete)**: Antena dipolo ou monopolo compacta
- **Receptor (base)**: Antena de alto ganho (Yagi, parabólica)

### Preciso de licença para operar LoRa?

LoRa opera em bandas ISM (Industrial, Scientific and Medical) que geralmente não requerem licença, mas há limites de potência e duty cycle. Verifique a regulamentação local (ANATEL no Brasil, FCC nos EUA, etc.).

## Conectividade

### Nenhuma porta serial aparece

**Soluções:**

1. Verifique se o dispositivo está conectado
2. Tente outra porta USB
3. Instale/atualize drivers
4. No Linux: `dmesg | tail` após conectar
5. No Windows: Gerenciador de Dispositivos

### "Could not open port"

**Causas comuns:**

- Outra aplicação usando a porta (Arduino IDE, minicom)
- Falta de permissões (Linux)
- Driver incorreto

**Solução:**

- Feche outras aplicações
- Verifique permissões
- Reinicie o computador

### Dados não chegam

**Checklist:**

1. ✅ Transmissor está ligado?
2. ✅ Frequência configurada corretamente?
3. ✅ Baud rate = 115200?
4. ✅ Antenas conectadas?
5. ✅ Dentro do alcance?

### RSSI muito baixo

**Melhorias:**

- Use antenas melhores
- Aumente TX power (máx 20 dBm)
- Eleve a antena receptora
- Reduza obstáculos
- Aumente Spreading Factor

### Muitos pacotes perdidos

**Soluções:**

- Reduza taxa de transmissão
- Aumente Coding Rate
- Verifique interferências (WiFi, outros dispositivos)
- Melhore antenas
- Aproxime transmissor e receptor

## Performance

### O mapa está lento

**Otimizações:**

- Limite o número de marcadores exibidos
- Use clustering de marcadores
- Reduza frequência de atualização
- Feche outras abas do navegador

### Memória aumentando constantemente

**Solução temporária:**

- Reinicie o servidor periodicamente
- Limpe o cache do navegador

**Solução definitiva:**

- Implemente limite de marcadores no mapa
- Use paginação na tabela

### Posso receber dados de múltiplos dispositivos?

Sim, o sistema já suporta foguete (#100) e satélite (#261). Para adicionar mais:

1. Defina novo TEAM_ID (ex: #102)
2. Adicione evento WebSocket no `app.py`
3. Crie nova página HTML
4. Adicione handler no `app.js`

Veja [Adicionando Funcionalidades](development.md#adicionando-funcionalidades).

### Qual a latência típica?

```
Transmissão LoRa:    100-500ms
Leitura Serial:      <50ms
Processamento:       <10ms
WebSocket:           <50ms
Render navegador:    50-200ms
─────────────────────────────
Total:               ~0.5-1s
```

## Desenvolvimento

### Como contribuo para o projeto?

1. Fork o repositório
2. Crie uma branch (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças
4. Push para o fork
5. Abra um Pull Request

Veja o [Guia de Desenvolvimento](development.md#contribuindo).

### Como adiciono um novo campo de dados?

Veja o tutorial completo em [Adicionando Novo Campo](development.md#1-adicionar-novo-campo-de-telemetria).

### Como testo sem hardware?

Use um mock serial ou crie portas virtuais:

```bash
# Linux - criar par de portas virtuais
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

Veja [Testando Mudanças](development.md#testando-mudanças).

### Posso usar outro framework web?

Sim! A arquitetura é modular. Você pode:

- Usar Django/FastAPI em vez de Flask
- Usar React/Vue em vez de templates Jinja2
- Implementar em Node.js

### Como faço debug?

Veja o guia completo de [Debugging](development.md#debugging).

**Dica rápida:**

```python
# Adicione prints
print(f"Debug: {variavel}")

# Ou use breakpoints
import pdb; pdb.set_trace()
```

### Onde está a documentação da API?

Veja [Documentação da API](api.md) completa.

### Posso usar HTTPS?

Sim! Configure certificados SSL:

```python
socketio.run(app,
    host='0.0.0.0',
    port=5000,
    ssl_context=('cert.pem', 'key.pem'))
```

Ou use um proxy reverso (nginx, Apache).

## Troubleshooting Avançado

### Como vejo mensagens WebSocket em tempo real?

No Chrome/Edge:

1. Abra DevTools (F12)
2. Aba "Network"
3. Filtro "WS" (WebSocket)
4. Clique na conexão socket.io
5. Veja aba "Messages"

### Como monitoro a porta serial diretamente?

**Linux:**

```bash
screen /dev/ttyACM0 115200
# ou
cat /dev/ttyACM0
```

**Windows:**

- Use PuTTY ou Arduino Serial Monitor

### O navegador não abre automaticamente

Abra manualmente:

```
http://localhost:5000
```

Ou desabilite o auto-open comentando:

```python
# open_browser(port)
```

### Erro "ImportError: No module named flask"

```bash
# Verifique se está no ambiente virtual
which python

# Reinstale dependências
pip install -r requirements.txt
```

### Como atualizo as dependências?

```bash
pip install --upgrade flask flask-socketio pyserial
```

⚠️ Teste após atualizar para garantir compatibilidade.

## Suporte Adicional

### Não encontrei minha dúvida aqui

1. Consulte a [Documentação completa](README.md)
2. Procure em [Issues existentes](../../issues)
3. Abra uma nova issue descrevendo:
   - Seu sistema operacional
   - Versão do Python
   - Mensagens de erro completas
   - Passos para reproduzir

### Como reporto um bug?

Abra uma issue no GitHub com:

```markdown
**Descrição do bug**
Descrição clara do problema

**Passos para reproduzir**

1. ...
2. ...

**Comportamento esperado**
O que deveria acontecer

**Screenshots**
Se aplicável

**Ambiente:**

- OS: [e.g. Ubuntu 22.04]
- Python: [e.g. 3.10.0]
- Navegador: [e.g. Chrome 120]
```

### Onde posso aprender mais sobre LoRa?

- [LoRa Alliance](https://lora-alliance.org/)
- [Semtech Documentation](https://www.semtech.com/lora)
- [The Things Network](https://www.thethingsnetwork.org/)

### Onde aprendo sobre foguetemodelismo?

- [NAR (National Association of Rocketry)](https://www.nar.org/)
- [Tripoli Rocketry Association](https://www.tripoli.org/)
- Clubes locais de foguetemodelismo

---

## Ainda tem dúvidas?

- 📧 Abra uma [Issue](../../issues/new)
- 💬 Participe das [Discussions](../../discussions)
- 📚 Leia a [Documentação completa](README.md)

---

[← Voltar à Documentação](README.md)
