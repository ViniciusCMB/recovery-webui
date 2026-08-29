# Guia de Instalação

Este guia detalha o processo de instalação do Recovery WebUI em diferentes sistemas operacionais.

## Índice

- [Requisitos do Sistema](#requisitos-do-sistema)
- [Instalação no Linux](#instalação-no-linux)
- [Instalação no Windows](#instalação-no-windows)
- [Instalação no macOS](#instalação-no-macos)
- [Configuração de Permissões](#configuração-de-permissões)
- [Verificação da Instalação](#verificação-da-instalação)
- [Problemas Comuns](#problemas-comuns)

## Requisitos do Sistema

### Software Necessário

- Python 3.7 ou superior
- pip (gerenciador de pacotes Python)
- Navegador web moderno (Chrome, Firefox, Edge, Safari)

### Hardware Necessário

- Porta USB disponível
- Módulo LoRa receptor (compatível com comunicação serial)
- Mínimo 512 MB de RAM
- 100 MB de espaço em disco

## Instalação no Linux

### Ubuntu / Debian

1. **Atualize o sistema:**

```bash
sudo apt update
sudo apt upgrade
```

2. **Instale o Python e pip (se ainda não estiver instalado):**

```bash
sudo apt install python3 python3-pip
```

3. **Clone o repositório:**

```bash
git clone <url-do-repositorio>
cd recovery-webui
```

4. **Instale as dependências:**

```bash
pip3 install -r requirements.txt
```

### Arch Linux / Manjaro

1. **Instale o Python:**

```bash
sudo pacman -S python python-pip
```

2. **Clone e instale:**

```bash
git clone <url-do-repositorio>
cd recovery-webui
pip install -r requirements.txt
```

### Fedora / RHEL / CentOS

1. **Instale o Python:**

```bash
sudo dnf install python3 python3-pip
```

2. **Clone e instale:**

```bash
git clone <url-do-repositorio>
cd recovery-webui
pip3 install -r requirements.txt
```

## Instalação no Windows

### Usando o Instalador Python

1. **Instale o Python:**
   - Baixe o instalador do [python.org](https://www.python.org/downloads/)
   - Durante a instalação, marque "Add Python to PATH"
   - Instale o Python

2. **Verifique a instalação:**

```cmd
python --version
pip --version
```

3. **Clone o repositório:**

```cmd
git clone <url-do-repositorio>
cd recovery-webui
```

4. **Instale as dependências:**

```cmd
pip install -r requirements.txt
```

### Drivers Serial no Windows

Para módulos CH340/CH341:

- Baixe o driver em [CH340 Drivers](http://www.wch-ic.com/downloads/CH341SER_EXE.html)
- Execute o instalador
- Reinicie o computador

Para módulos CP2102:

- Baixe o driver em [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- Execute o instalador
- Reinicie o computador

## Instalação no macOS

### Usando Homebrew

1. **Instale o Homebrew (se não tiver):**

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

2. **Instale o Python:**

```bash
brew install python
```

3. **Clone o repositório:**

```bash
git clone <url-do-repositorio>
cd recovery-webui
```

4. **Instale as dependências:**

```bash
pip3 install -r requirements.txt
```

### Drivers Serial no macOS

Para módulos CH340:

- Baixe o driver compatível com sua versão do macOS
- Instale o driver
- Reinicie o computador
- Autorize o driver em "Preferências do Sistema" > "Segurança e Privacidade"

## Configuração de Permissões

### Linux - Permissões da Porta Serial

Adicione seu usuário ao grupo `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Ou para algumas distribuições:

```bash
sudo usermod -a -G uucp $USER
```

**Importante:** Faça logout e login novamente para as mudanças terem efeito.

### Verificar Portas Disponíveis

**Linux:**

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

**Windows:**

- Abra o "Gerenciador de Dispositivos"
- Procure em "Portas (COM & LPT)"

**macOS:**

```bash
ls /dev/tty.*
```

## Verificação da Instalação

### Teste das Dependências

```bash
python3 -c "import flask; import flask_socketio; import serial; print('✓ Todas as dependências instaladas!')"
```

### Teste da Porta Serial

```bash
cd src
python3 -c "from modules import list_ports; print(list_ports())"
```

Deve listar as portas seriais disponíveis.

### Primeiro Teste

1. Conecte o módulo LoRa receptor
2. Execute a aplicação:

```bash
cd app
python3 app.py
```

3. Selecione a porta serial
4. Verifique se o navegador abre automaticamente
5. Se não abrir, acesse manualmente: `http://localhost:5000`

## Problemas Comuns

### "Permission denied" no Linux

**Problema:** Não consegue acessar a porta serial.

**Solução:**

```bash
sudo usermod -a -G dialout $USER
# Faça logout e login novamente
```

Ou execute com sudo (não recomendado para uso regular):

```bash
sudo python3 app.py
```

### "Port already in use"

**Problema:** A porta 5000 já está em uso.

**Solução:** Modifique a porta no arquivo `app.py`:

```python
port = 5001  # ou outra porta disponível
socketio.run(app, host='0.0.0.0', port=port)
```

### "Module not found"

**Problema:** Faltam dependências Python.

**Solução:**

```bash
pip3 install -r requirements.txt --user
```

Ou use um ambiente virtual:

```bash
python3 -m venv venv
source venv/bin/activate  # Linux/macOS
# ou
venv\Scripts\activate  # Windows
pip install -r requirements.txt
```

### Porta serial não aparece

**Problema:** O sistema não detecta o dispositivo USB.

**Soluções:**

1. Verifique se o dispositivo está conectado corretamente
2. Tente outra porta USB
3. Instale/atualize os drivers apropriados
4. No Linux, verifique com `dmesg | tail` após conectar o dispositivo

### Erro "Bad file descriptor"

**Problema:** Outra aplicação está usando a porta serial.

**Solução:**

- Feche outras aplicações que possam estar usando a porta (Arduino IDE, minicom, screen, etc.)
- Desconecte e reconecte o dispositivo
- Reinicie o computador se necessário

## Ambiente Virtual (Recomendado)

Para isolar as dependências do projeto:

### Criar ambiente virtual

```bash
python3 -m venv venv
```

### Ativar o ambiente

**Linux/macOS:**

```bash
source venv/bin/activate
```

**Windows:**

```cmd
venv\Scripts\activate
```

### Instalar dependências

```bash
pip install -r requirements.txt
```

### Desativar ambiente

```bash
deactivate
```

## Instalação para Desenvolvimento

Se você planeja contribuir para o projeto:

```bash
# Clone o repositório
git clone <url-do-repositorio>
cd recovery-webui

# Crie um ambiente virtual
python3 -m venv venv
source venv/bin/activate

# Instale as dependências
pip install -r requirements.txt

# Instale ferramentas de desenvolvimento (opcional)
pip install black flake8 pytest
```

## Próximos Passos

Após a instalação bem-sucedida:

1. Consulte o [README principal](../README.md) para uso básico
2. Leia a [Documentação da API](api.md) para integração
3. Veja a [Arquitetura do Sistema](architecture.md) para entender o funcionamento interno
4. Confira o [Guia de Desenvolvimento](development.md) se quiser contribuir

## Suporte

Se você encontrar problemas não listados aqui:

1. Verifique as [Issues](../../issues) no GitHub
2. Consulte os logs da aplicação para mensagens de erro
3. Abra uma nova issue com detalhes do problema e seu ambiente

---

[← Voltar ao README](../README.md)
