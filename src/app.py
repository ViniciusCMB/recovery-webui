#!/usr/bin/env python3
"""
Recovery WebUI — Interface de telemetria para receptor LoRa.

Modos de operação:
  --help        Exibe ajuda
  --debug       Ativa logs detalhados
  --cli         Modo terminal interativo (sem Flask/SocketIO)
  --simulation  Usa FakeCom (dados sinteticos em vez de serial real)

Uso:
  python src/app.py                          # Modo Flask (padrao)
  python src/app.py --cli                    # Modo terminal interativo
  python src/app.py --cli --simulation       # CLI com dados sinteticos
  python src/app.py --cli --simulation --debug  # CLI + sintetico + verbose
"""

from datetime import datetime
from threading import Lock, Thread
from typing import Callable, Optional
import logging
import os
import subprocess
import sys
import time

from flask import Flask, render_template, request
from flask_socketio import SocketIO
from simple_term_menu import TerminalMenu

from modules import BaseCom, FakeCom
from receiver import Receiver, CSV_HEADER, parse_packet


# ══════════════════════════════════════════════════════════════════════════
# Argumentos de linha de comando
# ══════════════════════════════════════════════════════════════════════════

debug_mode = False
simulation_mode = False
cli_mode = False

for arg in sys.argv[1:]:
    if arg == "--help":
        print(f"Uso: python {sys.argv[0]} [opcoes]")
        print()
        print("Opcoes:")
        print("  --help        Exibe esta ajuda")
        print("  --debug       Ativa logs detalhados (DEBUG)")
        print("  --cli         Modo terminal interativo (sem web)")
        print("  --simulation  Usa dados sinteticos (FakeCom)")
        sys.exit(0)
    elif arg == "--debug":
        debug_mode = True
    elif arg == "--cli":
        cli_mode = True
    elif arg == "--simulation":
        simulation_mode = True

# ══════════════════════════════════════════════════════════════════════════
# Logging estruturado
# ══════════════════════════════════════════════════════════════════════════

app_name = "CLI" if cli_mode else "WebApp"
main_logger = logging.getLogger(app_name)
main_logger.setLevel(logging.DEBUG if debug_mode else logging.INFO)

log_dir = os.path.join(os.path.dirname(__file__), "..", "logs")
os.makedirs(log_dir, exist_ok=True)

current_day = datetime.now().strftime("%Y_%m_%d")
file_handler = logging.FileHandler(
    os.path.join(log_dir, f"webapp_{current_day}.log"),
    mode="a",
    encoding="utf-8",
)
file_handler.setFormatter(
    logging.Formatter(
        "[%(asctime)s] %(levelname)-8s (%(name)s): %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
)
main_logger.addHandler(file_handler)

if not cli_mode:
    console = logging.StreamHandler()
    console.setFormatter(
        logging.Formatter(
            "[%(asctime)s] %(levelname)-8s (%(name)s): %(message)s",
            datefmt="%H:%M:%S",
        )
    )
    main_logger.addHandler(console)

antenna_logger = main_logger.getChild("Antenna")
ws_logger = main_logger.getChild("WS")

main_logger.info("--- NOVA EXECUCAO ---")
if debug_mode:
    main_logger.debug("modo debug ativado")
if simulation_mode:
    main_logger.debug("modo simulacao ativado (FakeCom)")
if cli_mode:
    main_logger.debug("modo CLI ativado")

# ══════════════════════════════════════════════════════════════════════════
# Interface serial + Receiver
# ══════════════════════════════════════════════════════════════════════════

if simulation_mode:
    com: BaseCom = FakeCom(antenna_logger)
else:
    if BaseCom is None:
        main_logger.error(
            "pyserial nao instalado. Use --simulation ou instale: pip install pyserial"
        )
        sys.exit(1)
    com = BaseCom(antenna_logger)
data_dir = os.path.join(os.path.dirname(__file__), "..", "data")
receiver = Receiver(com, logger=antenna_logger, data_dir=data_dir)


# ══════════════════════════════════════════════════════════════════════════
# MODO FLASK (padrao)
# ══════════════════════════════════════════════════════════════════════════

if not cli_mode:
    web_dir = os.path.join(os.path.dirname(__file__), "..", "web")

    app = Flask(
        __name__,
        template_folder=os.path.join(web_dir, "templates"),
        static_folder=os.path.join(web_dir, "static"),
    )
    app.config["SESSION_COOKIE_PATH"] = "/"

    socketio = SocketIO(app)

    _background_thread = None
    _background_lock = Lock()


    @app.route("/")
    def index():
        return render_template("index.html")


    @app.route("/satellite")
    def satellite():
        return render_template("satellite.html")


    @socketio.on("connect")
    def on_connect():
        ws_logger.info("cliente conectado")
        global _background_thread
        with _background_lock:
            if _background_thread is None:
                timestamp = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
                csv_path = os.path.join(data_dir, f"data_{timestamp}.csv")

                def _run():
                    receiver.open()
                    receiver.send_mission_id_table(socketio_instance=socketio)
                    receiver.capture_loop(csv_path, socketio_instance=socketio)

                _background_thread = socketio.start_background_task(_run)
                ws_logger.info(f"background thread iniciada -> {csv_path}")


    @socketio.on("disconnect")
    def on_disconnect():
        ws_logger.info("cliente desconectado")


# ══════════════════════════════════════════════════════════════════════════
# MODO CLI
# ══════════════════════════════════════════════════════════════════════════

cli_data_flag = False        # controla o loop da thread de captura
cli_display_flag = False     # exibe dados no terminal
cli_daemon_thread = None
cli_thread_lock = Lock()
cli_write_interval = 0.5     # segundos entre leituras


def _cli_clear():
    subprocess.call("clear" if os.name == "posix" else "cls")


def _cli_data_worker():
    """Thread secundaria que captura dados da serial para CSV."""
    main_logger.info("thread de captura de dados inicializada")
    global cli_data_flag, cli_display_flag

    timestamp = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
    csv_path = os.path.join(data_dir, f"data_{timestamp}.csv")

    with open(csv_path, "w") as f:
        f.write(f"{CSV_HEADER}\n")

    while cli_data_flag:
        response = receiver.com.read_response()
        if not response:
            time.sleep(cli_write_interval)
            continue

        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        with open(csv_path, "a") as f:
            f.write(f"{now},{response}\n")

        if cli_display_flag:
            result = parse_packet(response)
            if result is None:
                continue
            team_id, fields = result
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

        time.sleep(cli_write_interval)

    main_logger.info("thread de captura de dados finalizada")


# ── Menus CLI ────────────────────────────────────────────────────────────


def cli_main_menu() -> Optional[Callable]:
    _cli_clear()

    status = "conectado" if receiver.check_connected() else "desconectado"
    captura = "ativada" if cli_data_flag else "desativada"
    port = receiver.get_port() or "indefinida"

    title = f"""
    Menu Principal:
        Porta     -> {port}
        Baudrate  -> {receiver.get_baudrate()} bauds
        Timeout   -> {receiver.get_timeout()} sec

        Serial    -> {status}
        Captura   -> {captura}
        Intervalo -> {cli_write_interval} sec
    """

    connected = receiver.check_connected()
    options = [
        "Atualizar status",
        "Desativar serial" if connected else "Ativar serial",
        "Configurar serial",
        "Monitorar serial",
        "Sair",
    ]

    idx = TerminalMenu(options, title=title).show()

    if idx == 0:
        return cli_main_menu
    elif idx == 1:
        return cli_serial_off if connected else cli_serial_on
    elif idx == 2:
        return cli_config_menu
    elif idx == 3:
        return cli_monitor
    elif idx == 4 or idx is None:
        main_logger.info("CLI finalizado")
        return None
    return cli_main_menu


def cli_serial_on() -> Optional[Callable]:
    if receiver.open():
        global cli_daemon_thread, cli_data_flag
        cli_data_flag = True
        cli_daemon_thread = Thread(target=_cli_data_worker, daemon=True)
        cli_daemon_thread.start()
    else:
        main_logger.error("nao foi possivel abrir a conexao serial")
    return cli_main_menu


def cli_serial_off() -> Optional[Callable]:
    global cli_data_flag
    cli_data_flag = False
    receiver.close()
    main_logger.info("serial desativada")
    return cli_main_menu


def cli_config_menu() -> Optional[Callable]:
    if receiver.check_connected():
        main_logger.warning("desative a serial antes de configurar")
        return cli_main_menu

    title = "Configuracao Serial:"
    options = ["Selecionar porta", "Selecionar baudrate", "Selecionar timeout", "Voltar"]
    idx = TerminalMenu(options, title=title).show()

    if idx == 0:
        return _cli_config_port
    elif idx == 1:
        return _cli_config_baudrate
    elif idx == 2:
        return _cli_config_timeout
    return cli_main_menu


def _cli_config_port() -> Optional[Callable]:
    ports = receiver.get_port_options()
    options = list(ports) + ["Nao mudar"]
    title = f"Selecione a porta: {'(nenhuma encontrada)' if not ports else ''}"
    idx = TerminalMenu(options, title=title).show()
    if idx is not None and idx < len(ports):
        receiver.set_port(ports[idx])
    return cli_config_menu


def _cli_config_baudrate() -> Optional[Callable]:
    rates = [str(b) for b in receiver.get_baudrate_options()]
    options = rates + ["Nao mudar"]
    idx = TerminalMenu(options, title="Selecione o baudrate:").show()
    if idx is not None and idx < len(rates):
        receiver.set_baudrate(int(rates[idx]))
    return cli_config_menu


def _cli_config_timeout() -> Optional[Callable]:
    timeouts = ["0", "0.25", "0.5", "1.0", "2.0", "2.5", "5.0", "7.5", "10"]
    options = timeouts + ["Nao mudar"]
    idx = TerminalMenu(options, title="Selecione o timeout:").show()
    if idx is not None and idx < len(timeouts):
        receiver.set_timeout(float(timeouts[idx]))
    return cli_config_menu


def cli_monitor() -> Optional[Callable]:
    if not receiver.check_connected():
        main_logger.error("serial desconectada — impossivel monitorar")
        return cli_main_menu

    _cli_clear()
    global cli_display_flag
    cli_display_flag = True
    input("Pressione <Enter> para parar o monitoramento.\n\n")
    cli_display_flag = False
    return cli_main_menu


# ══════════════════════════════════════════════════════════════════════════
# Entry-point
# ══════════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    if cli_mode:
        next_state: Optional[Callable] = cli_main_menu()
        while next_state:
            next_state = next_state()
    else:
        socketio.run(app, host="0.0.0.0", port=5000, debug=debug_mode)
