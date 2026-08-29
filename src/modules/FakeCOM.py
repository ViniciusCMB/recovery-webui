"""
Módulo de comunicação serial simulada para testes sem hardware.

Fornece a classe FakeCom que emula a comunicação serial do receptor LoRa
com dados de telemetria sintéticos no formato v2.0 (24 campos).
"""

try:
    from .SerialCOM import BaseCom
except ImportError:
    # pyserial nao instalado — FakeCom funciona sem serial real
    class BaseCom:  # type: ignore
        """Stub para quando pyserial nao esta disponivel."""
        def __init__(self, *args, **kwargs):
            raise ImportError(
                "pyserial nao instalado. Use --simulation ou pip install pyserial"
            )

import logging
from typing import Optional


class FakeCom(BaseCom):
    """
    Emulador de comunicação serial para testes sem antena/receptor.

    Gera dados sintéticos no formato v2.0 com TEAM_ID #213 (satélite),
    simulando um pacote a cada chamada de read_response() em ciclo.
    """

    def __init__(self, logger_: logging.Logger = logging.getLogger(__name__)):
        self._is_open = False
        self._fake_port = "fake-1"
        self._fake_baudrate = 115200
        self._fake_timeout = 1.0

        # Dados sintéticos no formato v2.0 (24 campos)
        # TEAM_ID,millis,count,altp,temp,umi,p,gx,gy,gz,ax,ay,az,
        #   vz,maxAltitude,state,hora,data,alt,lat,lon,sat,parachute,rssi
        self._fake_lines = [
            # Subindo
            "#213,1000,1,0.00,25.30,60.50,960.00,0.01,-0.02,0.01,0.10,0.20,-9.80,0.00,0.00,0,143000,22072026,478.00,-21.94305,-48.95409,10,0,-45",
            "#213,1100,2,5.20,25.32,60.48,959.88,0.02,-0.03,0.02,0.11,0.21,-9.79,0.05,5.20,0,143001,22072026,483.20,-21.94304,-48.95408,10,0,-45",
            "#213,1200,3,10.50,25.28,60.52,959.72,0.01,-0.01,0.01,0.09,0.19,-9.78,0.10,10.50,0,143002,22072026,488.50,-21.94303,-48.95407,10,0,-46",
            "#213,1300,4,15.80,25.25,60.55,959.55,0.03,-0.04,0.02,0.12,0.22,-9.77,0.15,15.80,0,143003,22072026,493.80,-21.94302,-48.95406,10,0,-46",
            "#213,1400,5,21.10,25.20,60.58,959.38,0.02,-0.02,0.01,0.10,0.20,-9.76,0.20,21.10,0,143004,22072026,499.10,-21.94301,-48.95405,10,0,-46",
            "#213,1500,6,26.40,25.18,60.60,959.20,0.01,-0.03,0.02,0.11,0.21,-9.75,0.25,26.40,0,143005,22072026,504.40,-21.94300,-48.95404,10,0,-47",
            "#213,1600,7,31.70,25.15,60.62,959.02,0.02,-0.01,0.01,0.09,0.19,-9.74,0.30,31.70,0,143006,22072026,509.70,-21.94299,-48.95403,10,0,-47",
            "#213,1700,8,37.00,25.12,60.65,958.85,0.01,-0.02,0.02,0.10,0.20,-9.73,0.35,37.00,0,143007,22072026,515.00,-21.94298,-48.95402,10,0,-47",
            "#213,1800,9,42.30,25.08,60.68,958.68,0.03,-0.03,0.01,0.11,0.21,-9.72,0.40,42.30,0,143008,22072026,520.30,-21.94297,-48.95401,10,0,-48",
            "#213,1900,10,47.60,25.05,60.70,958.50,0.02,-0.02,0.02,0.10,0.20,-9.71,0.45,47.60,0,143009,22072026,525.60,-21.94296,-48.95400,10,0,-48",
            # Apogeu (altp ~500m + 478m base = 978m)
            "#213,5000,35,500.00,22.80,62.10,940.00,0.01,-0.01,0.01,0.08,0.18,-9.70,0.10,500.00,3,143030,22072026,978.00,-21.94280,-48.95385,9,0,-50",
            # Descendo com paraquedas
            "#213,8000,60,400.00,23.50,61.50,945.00,0.02,-0.02,0.01,0.09,0.19,-8.50,-5.00,500.00,5,143100,22072026,878.00,-21.94260,-48.95370,8,1,-52",
            "#213,11000,85,300.00,24.10,61.00,950.00,0.01,-0.01,0.02,0.08,0.18,-7.80,-4.50,500.00,5,143130,22072026,778.00,-21.94240,-48.95355,8,1,-55",
            "#213,14000,110,200.00,24.60,60.50,955.00,0.01,-0.02,0.01,0.07,0.17,-8.10,-3.80,500.00,6,143200,22072026,678.00,-21.94220,-48.95340,9,1,-58",
            "#213,17000,135,100.00,25.00,60.00,960.00,0.02,-0.01,0.01,0.08,0.18,-8.30,-2.50,500.00,6,143230,22072026,578.00,-21.94200,-48.95325,9,1,-60",
            "#213,20000,160,50.00,25.20,59.80,962.00,0.01,-0.01,0.01,0.07,0.17,-8.50,-1.00,500.00,7,143300,22072026,528.00,-21.94180,-48.95310,10,1,-62",
            # Solo
            "#213,23000,185,0.00,25.30,59.50,965.00,0.01,-0.02,0.01,0.06,0.16,-9.80,0.00,500.00,0,143330,22072026,478.00,-21.94160,-48.95295,10,0,-65",
        ]
        self._index = 0

        # Não chama super().__init__ — não precisamos de serial real
        self.logger = logger_
        self.logger.debug("antena sintética criada")

    # ── Controle de conexão ──────────────────────────────────────────────

    def open(self) -> bool:
        self._is_open = True
        self.logger.info("conexão serial simulada aberta")
        return True

    def close(self):
        self._is_open = False
        self.logger.info("conexão serial simulada fechada")

    def check_connected(self) -> bool:
        return self._is_open

    # ── Leitura ──────────────────────────────────────────────────────────

    def read_response(self) -> Optional[str]:
        if not self._is_open:
            self.logger.error("conexão serial simulada fechada")
            return None

        line = self._fake_lines[self._index]
        self._index = (self._index + 1) % len(self._fake_lines)
        return line

    # ── Getters / Setters ────────────────────────────────────────────────

    def get_port(self) -> Optional[str]:
        return self._fake_port

    def get_baudrate(self) -> int:
        return self._fake_baudrate

    def get_timeout(self) -> float:
        return self._fake_timeout

    def set_port(self, port: str) -> None:
        self.logger.info(f"definindo porta simulada como {port}")
        self._fake_port = port

    def set_baudrate(self, baudrate: int) -> None:
        self.logger.info(f"definindo baudrate simulado como {baudrate}")
        self._fake_baudrate = baudrate

    def set_timeout(self, timeout: float) -> None:
        self.logger.info(f"definindo timeout simulado como {timeout}")
        self._fake_timeout = timeout

    # ── Opções ───────────────────────────────────────────────────────────

    def get_port_options(self) -> list:
        return ["fake-1", "fake-2", "fake-3"]
