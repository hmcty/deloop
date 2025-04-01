"""Handles a UART stream for Mk0 devices."""

import argparse
import json
import logging
from contextlib import contextmanager
from pathlib import Path
from typing import Final, Generator

import log_pb2
import serial
import stream_pb2
from serial.threaded import Protocol, ReaderThread
from serial.tools import list_ports
from tabulate import tabulate

LOG_TABLE_FILE: Final[Path] = Path(__file__).parent / "log_table.json"

logger = logging.getLogger(__name__)


class Mk0Stream(Protocol):

    MAGIC_BYTE: Final[int] = 0xEB

    transport: ReaderThread
    log_table: dict[str, str]

    start_byte_received: bool
    data_remaining: int | None
    buffer: bytearray

    def __init__(self):
        self.transport = None
        with open(LOG_TABLE_FILE, "r") as file:
            self.log_table = json.load(file)

        self.start_byte_received = False
        self.data_remaining = None
        self.buffer = bytearray()

    def connection_made(self, transport: ReaderThread) -> None:
        logger.info("Connected to device.")
        self.transport = transport
        self.transport.serial.reset_input_buffer()

    def connection_lost(self, exc) -> None:
        self.transport = None
        super().connection_lost(exc)

    def data_received(self, data: bytes) -> None:
        # First, read until our magic byte is found.
        if not self.start_byte_received:
            while len(data) > 0:
                start, data = data[0], data[1:]
                if start == self.MAGIC_BYTE:
                    self.start_byte_received = True
                    break

            if len(data) == 0 or not self.start_byte_received:
                return

        # Next byte is the expected length of the packet.
        if self.data_remaining is None:
            self.data_remaining = data[0]
            data = data[1:]
            if len(data) == 0:
                return

        # Read the specified number of bytes.
        read_len = min(len(data), self.data_remaining)
        self.buffer.extend(data[:read_len])
        data = data[read_len:]
        self.data_remaining -= read_len

        # Once packet has been fully read, clear start flag.
        if self.data_remaining == 0:
            self.handle_packet(bytes(self.buffer))
            self.start_byte_received = False
            self.data_remaining = None
            self.buffer.clear()

        # Recurse on start of the next packet if included.
        if len(data) > 0:
            self.data_received(data)

    def parse_log_args(
        self,
        args: list[log_pb2.LogRecord.Arg],
    ) -> list[int | float]:
        arg_values = []
        for arg in args:
            if arg.HasField("u32"):
                arg_values.append(arg.u32)
            elif arg.HasField("f32"):
                arg_values.append(arg.f32)
            elif arg.HasField("i32"):
                arg_values.append(arg.i32)

        return arg_values

    def handle_log(self, log: log_pb2.LogRecord) -> None:
        level = log_pb2.LogLevel.Name(log.level)
        msg = self.log_table.get(str(log.hash))
        if msg is None:
            logger.warning(f"Unknown hash: {log.hash}")
            return

        args = self.parse_log_args(log.args)
        logger.log(logging.getLevelName(level), msg, *args)

    def handle_packet(self, packet: bytes) -> None:
        try:
            # print(f"Received packet: {packet}")
            stream = stream_pb2.StreamPacket()
            stream.ParseFromString(packet)
            if stream.HasField("log"):
                self.handle_log(stream.log)

        except Exception as e:
            logger.exception(f"Error: {e}")


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


def add_uart_args(parser: argparse.ArgumentParser) -> None:
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


@contextmanager
def open_uart_stream(
    args: argparse.Namespace,
) -> Generator[Mk0Stream, None, None]:
    port = args.port or select_port()
    ser = serial.serial_for_url(port, args.baudrate, timeout=1)
    with ReaderThread(ser, Mk0Stream) as stream:
        yield stream
