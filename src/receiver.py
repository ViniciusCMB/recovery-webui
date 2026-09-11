"""
Módulo Receiver — parser de telemetria v2.0 e captura de dados.

Centraliza a lógica de parse de pacotes, mapeamento de TEAM_IDs,
construção de eventos WebSocket e o loop de captura com gravação CSV.

Uso:
    from modules import BaseCom, FakeCom
    from receiver import Receiver

    com = FakeCom()
    rx = Receiver(com)
    rx.open()
    team_id, fields = rx.read_and_parse()
    rx.capture_loop("data.csv", socketio=socketio_instance)
"""

from datetime import datetime
from typing import Optional, Callable
import logging
import os
import time

from flask_socketio import SocketIO

from modules import BaseCom


# ══════════════════════════════════════════════════════════════════════════
# Constantes do protocolo v2.0
# ══════════════════════════════════════════════════════════════════════════

CSV_HEADER = (
    "NOW,TEAM_ID,millis,count,altp,temp,umi,p,"
    "gx,gy,gz,ax,ay,az,vz,maxAltitude,state,"
    "hora,data,alt,lat,lon,sat,parachute,rssi"
)

# TEAM_ID → (evento_websocket, funcao_de_construcao_do_evento)
TEAM_MAP: dict[str, tuple[str, Callable]] = {}


def _rocket_event(fields: dict, now: str) -> dict:
    return {

        "team_id": fields["team_id"],
        "latitude": fields["lat"],
        "longitude": fields["lon"],
        "altura": fields["altp"],
        "satelites": fields["sat"],
        "temperatura": fields["temp"],
        "umidade": fields["umi"],
        "pressao": fields["press"],
        "rssi": fields["rssi"],
        "time": now,
    }


def _sat_event(fields: dict, now: str) -> dict:
    return {
        "team_id": fields["team_id"],
        "latitude": fields["lat"],
        "longitude": fields["lon"],
        "altura": fields["altp"],
        "satelites": fields["sat"],
        "temperatura": fields["temp"],
        "umidade": fields["umi"],
        "pressao": fields["press"],
        "rssi": fields["rssi"],
        "time": now,
    }


TEAM_MAP["#11"] = ("updateRocket", _rocket_event)
TEAM_MAP["#51"] = ("updateRocket", _rocket_event)
TEAM_MAP["#213"] = ("updateSat", _sat_event)


# ══════════════════════════════════════════════════════════════════════════
# Parser
# ══════════════════════════════════════════════════════════════════════════

def get_current_datetime() -> str:
    """Retorna timestamp atual no formato YYYY-MM-DD HH:MM:SS."""
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def parse_packet(
    response: str,
) -> Optional[tuple[str, dict[str, str]]]:
    """
    Faz o parse de um pacote v2.0 (24 campos).

    O simulate-receiver adiciona '#' ao final como marcador de integridade;
    o receiver real nao adiciona. Esta funcao aceita ambos os casos.

    Returns:
        (team_id, dict_campos) em caso de sucesso
        None                    em caso de falha
    """
    raw = response.strip()
    if raw.endswith("#"):
        raw = raw[:-1]

    fields = raw.split(",")
    if len(fields) < 24:
        return None

    (
        team_id,
        millis,
        count,
        altp,
        temp,
        umi,
        p,
        gx,
        gy,
        gz,
        ax,
        ay,
        az,
        vz,
        max_alt,
        state,
        hora,
        data_,
        alt,
        lat,
        lon,
        sat,
        parachute,
        rssi,
    ) = fields

    return team_id, {
        "team_id": team_id,
        "millis": millis,
        "count": count,
        "altp": altp,
        "temp": temp,
        "umi": umi,
        "press": p,
        "gx": gx,
        "gy": gy,
        "gz": gz,
        "ax": ax,
        "ay": ay,
        "az": az,
        "vz": vz,
        "maxAltitude": max_alt,
        "state": state,
        "hora": hora,
        "data": data_,
        "alt": alt,
        "lat": lat,
        "lon": lon,
        "sat": sat,
        "parachute": parachute,
        "rssi": rssi,
    }


# ══════════════════════════════════════════════════════════════════════════
# Classe Receiver
# ══════════════════════════════════════════════════════════════════════════

class Receiver:
    """
    Receptor de telemetria LoRa — encapsula serial, parser e captura.

    Pode ser usado tanto pelo modo CLI quanto pelo modo Flask.
    Aceita qualquer objeto com interface BaseCom (real ou FakeCom).
    """

    def __init__(
        self,
        com: BaseCom,
        logger: logging.Logger = logging.getLogger(__name__),
        data_dir: str = "data",
    ):
        self.com = com
        self.logger = logger
        self.data_dir = data_dir
        os.makedirs(data_dir, exist_ok=True)

    # ── Delegação serial ────────────────────────────────────────────────

    def open(self) -> bool:
        return self.com.open()

    def close(self):
        self.com.close()

    def check_connected(self) -> bool:
        return self.com.check_connected()

    def get_port(self) -> Optional[str]:
        return self.com.get_port()

    def get_baudrate(self) -> int:
        return self.com.get_baudrate()

    def get_timeout(self) -> float:
        return self.com.get_timeout()

    def set_port(self, port: str):
        self.com.set_port(port)

    def set_baudrate(self, baudrate: int):
        self.com.set_baudrate(baudrate)

    def set_timeout(self, timeout: float):
        self.com.set_timeout(timeout)

    def get_port_options(self) -> list:
        return self.com.get_port_options()

    def get_baudrate_options(self) -> list:
        return self.com.get_baudrate_options()

    def get_timeout_options(self) -> list:
        return self.com.get_timeout_options()

    # ── Leitura e parse ─────────────────────────────────────────────────

    def read_and_parse(self) -> Optional[tuple[str, dict[str, str]]]:
        """
        Le um pacote da serial e retorna (team_id, dict_campos).

        Returns None se nao houver dados ou o parse falhar.
        """
        response = self.com.read_response()
        if not response:
            return None
        result = parse_packet(response)
        if result is None:
            self.logger.warning(f"parse falhou para: {response[:60]}...")
            return None
        return result

    def build_event(self, team_id: str, fields: dict, now: str) -> Optional[dict]:
        """
        Constroi o payload do evento WebSocket apropriado para o TEAM_ID.

        Returns None se o TEAM_ID nao for reconhecido.
        """
        entry = TEAM_MAP.get(team_id)
        if entry is None:
            self.logger.debug(f"TEAM_ID nao mapeado: {team_id}")
            return None
        _, builder = entry
        return builder(fields, now)

    def get_event_name(self, team_id: str) -> Optional[str]:
        """Retorna o nome do evento SocketIO para o TEAM_ID."""
        entry = TEAM_MAP.get(team_id)
        return entry[0] if entry else None

    # ── Loop de captura ─────────────────────────────────────────────────

    def capture_loop(
        self,
        data_out_path: str,
        *,
        display: bool = False,
        socketio_instance=None,
        interval: float = 0.5,
    ):
        """
        Loop principal de captura de dados da serial.

        Args:
            data_out_path: Caminho do arquivo CSV.
            display: Exibe cada pacote no terminal.
            socketio_instance: Emite eventos WebSocket (modo Flask).
            interval: Segundos entre leituras.
        """
        with open(data_out_path, "w") as f:
            f.write(f"{CSV_HEADER}\n")

        self.logger.info(f"captura iniciada -> {data_out_path}")

        while True:
            try:
                response = self.com.read_response()
                if not response:
                    time.sleep(interval)
                    continue

                now = get_current_datetime()

                # Parse
                result = parse_packet(response)
                if result is None:
                    time.sleep(interval)
                    continue
                team_id, fields = result

                # CSV raw
                with open(data_out_path, "a") as f:
                    f.write(f"{now},{response}\n")

                # Display no terminal
                if display:
                    altp = fields.get("altp", "?")
                    lat = fields.get("lat", "?")
                    lon = fields.get("lon", "?")
                    rssi = fields.get("rssi", "?")
                    print(
                        f"[{now}] {team_id} "
                        f"altp={altp}m "
                        f"lat={lat} lon={lon} "
                        f"rssi={rssi}dBm"
                    )

                # WebSocket
                if socketio_instance is not None:
                    ev = self.build_event(team_id, fields, now)
                    if ev is not None:
                        event_name = self.get_event_name(team_id)
                        socketio_instance.emit(event_name, ev)

                time.sleep(interval)

            except Exception as exc:
                self.logger.error(f"erro no loop de captura: {exc}")
                time.sleep(interval)

    def send_mission_id_table(self, socketio_instance: SocketIO) -> bool:
        event_name: str = "id_table"
        event_content: dict = {
                    213: "satelite",
                    51: "foguete-2",
                    11: "foguete-1"
                }

        socketio_instance.emit(event_name, )
        return True
