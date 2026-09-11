
"""
Módulo de comunicação serial para o Recovery WebUI.

Fornece a classe BaseCom para comunicação serial real via pyserial
e a função module-level list_ports() para enumeração de portas seriais.
"""

import glob
import logging
import platform
import serial
import serial.tools.list_ports
import sys
from typing import Optional

logger = logging.getLogger(__name__)


class BaseCom:
    """Interface de comunicação serial com o receptor LoRa."""

    def __init__(self, logger_: logging.Logger = logger):
        self.logger = logger_
        self._serial = serial.Serial(
            port=None,
            baudrate=115200,
            timeout=1.0,
            xonxoff=False,
            rtscts=False,
            write_timeout=1.0,
            dsrdtr=False,
            inter_byte_timeout=None,
        )
        self.logger.info("interface de comunicação criada")

    # ── Controle de conexão ──────────────────────────────────────────────

    def open(self) -> bool:
        """Abre a conexão serial."""
        if self._serial.is_open:
            return True
        try:
            self._serial.open()
            self.logger.info(
                f"conexão serial aberta em {self._serial.port} "
                f"@ {self._serial.baudrate} baud"
            )
            return True
        except serial.SerialException as err:
            self.logger.error(f"erro ao abrir conexão serial: {err}")
            return False

    def close(self):
        """Fecha a conexão serial."""
        try:
            self._serial.close()
            self.logger.info("conexão serial fechada")
        except serial.SerialException as err:
            self.logger.error(f"erro ao fechar conexão serial: {err}")

    def check_connected(self) -> bool:
        """Verifica se a conexão serial está ativa."""
        return self._serial.is_open

    # ── Leitura / Escrita ────────────────────────────────────────────────

    def send_command(self, command: bytes) -> bool:
        """Envia um comando serial."""
        if not self.check_connected():
            return False
        try:
            self._serial.write(command)
            self.logger.debug(f"comando enviado: {command!r}")
            return True
        except serial.SerialException as err:
            self.logger.error(f"erro ao enviar comando: {err}")
            return False

    def read_response(self) -> Optional[str]:
        """
        Lê uma linha da serial.

        Returns:
            String decodificada (UTF-8) sem espaços/brancos, ou
            None se a serial não estiver conectada ou ocorrer erro.
        """
        if not self.check_connected():
            return None
        try:
            line = self._serial.readline()
            if not line:
                return None
            if(is_wsl()):
                return line.decode("latin-1").strip()
            else:
                return line.decode("utf-8").strip()
        except serial.SerialException as err:
            self.logger.error(f"erro ao ler da serial: {err}")
            return None

    # ── Getters / Setters ────────────────────────────────────────────────

    def get_port(self) -> Optional[str]:
        return self._serial.port

    def get_baudrate(self) -> int:
        return self._serial.baudrate

    def get_timeout(self) -> float:
        return self._serial.timeout if self._serial.timeout is not None else 1.0

    def set_port(self, port: str) -> None:
        self.logger.info(f"definindo porta como {port}")
        self._serial.port = port

    def set_baudrate(self, baudrate: int) -> None:
        self.logger.info(f"definindo baudrate como {baudrate}")
        self._serial.baudrate = baudrate

    def set_timeout(self, timeout: float) -> None:
        self.logger.info(f"definindo timeout como {timeout}")
        self._serial.timeout = timeout

    # ── Opções disponíveis ───────────────────────────────────────────────

    def get_port_options(self) -> list:
        """
        Lista todas as portas seriais disponíveis no sistema
        tentando abri-las.
        """
        if sys.platform.startswith(("linux", "cygwin")):
            candidates = glob.glob("/dev/tty[A-Za-z]*")
        elif sys.platform.startswith("win"):
            candidates = [f"COM{i+1}" for i in range(256)]
        else:
            self.logger.error("sistema operacional não suportado")
            return []

        available = []
        for port in candidates:
            try:
                s = serial.Serial(port)
                s.close()
                available.append(port)
            except (OSError, serial.SerialException):
                pass

        if not available:
            self.logger.warning("nenhuma porta serial encontrada")
        return available

    @staticmethod
    def get_baudrate_options() -> list:
        return list(serial.Serial.BAUDRATES)

    @staticmethod
    def get_timeout_options() -> list:
        return [0, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0, 7.5, 10]

    @staticmethod
    def list_ports() -> list:
        """
        Lista portas seriais via pyserial tools (filtra ACM no Linux).
        """
        if sys.platform.startswith("win"):
            return [p.device for p in serial.tools.list_ports.comports()]
        elif sys.platform.startswith(("linux", "cygwin")):
            return [
                p.device
                for p in serial.tools.list_ports.comports()
                if "/dev/ttyACM" in p.device
            ]
        return []


# ── Função module-level para compatibilidade ─────────────────────────────

def list_ports() -> list:
    """Atalho para BaseCom.list_ports()."""
    return BaseCom.list_ports()

# ── Reconhecer se estamos rodando no WSL ou no Linux nativo ──────────────

def is_wsl() -> bool:
    # 1. Check the kernel name string (Most reliable method)
    # WSL 1 typically includes "Microsoft"
    # WSL 2 typically includes "microsoft-standard-WSL"
    if "microsoft" in platform.uname().release.lower():
        return True

    # 2. Fallback check for certain container/minimal setups
    # Checks the system version file directly
    try:
        with open('/proc/version', 'r') as f:
            if 'microsoft' in f.read().lower():
                return True
    except FileNotFoundError:
        pass

    return False
