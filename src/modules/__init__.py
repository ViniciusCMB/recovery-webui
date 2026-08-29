from .FakeCOM import FakeCom

try:
    from .SerialCOM import BaseCom, list_ports
except ImportError:
    # pyserial nao instalado — FakeCom funciona, BaseCom nao
    BaseCom = None
    list_ports = None

__all__ = ["BaseCom", "FakeCom", "list_ports"]
