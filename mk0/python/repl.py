"""Primary user interface.

"""

import argparse

from tabulate import tabulate
# import ipython
import serial
from serial.tools import list_ports
from serial.threaded import Packetizer, ReaderThread
import time

import log_pb2
import stream_pb2


class Mk0Stream(Packetizer):

    def handle_packet(self, packet: bytes) -> None:
        stream = stream_pb2.StreamPacket()
        stream.ParseFromString(packet)
        print(stream)


def select_port() -> str:
    ports = list_ports.comports()
    print(tabulate(
        [(i, port.name, port.description) for i, port in enumerate(ports)],
        headers=["Index", "Device", "Description"],
    ) + "\n")
    while True:
        index = input("Select a port: ")
        try:
            index = int(index)
            if 0 <= index < len(ports):
                return ports[index].device

        except ValueError:
            pass

        print("Invalid selection. Please try again.")


def main(args: argparse.Namespace) -> None:
    if args.port is None:
        port = select_port()

    ser = serial.serial_for_url(port, args.baudrate, timeout=1)
    with ReaderThread(ser, Mk0Stream) as protocol:
        time.sleep(15.0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--port",
        type=str,
        help="Serial port to connect to.",
        default=None,
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        help="Baud rate for serial connection.",
        default=115200,
    )
    main(parser.parse_args())
