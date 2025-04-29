"""Handles a UART stream for Mk0 devices."""

import argparse
import importlib
import json
import logging
import struct
from contextlib import contextmanager
from typing import Final, Generator

import serial
from serial.threaded import Protocol, ReaderThread
from serial.tools import list_ports
from tabulate import tabulate

try:
    import command_pb2
    import log_pb2
    import stream_pb2
except ImportError as e:
    print(e)
    print("Missing protobuf modules. Run build script.")
    raise SystemExit

LOG_TABLE_FILE = importlib.resources.files("deloop_mk0") / "log_table.json"

logger = logging.getLogger(__name__)


class Mk0Stream(Protocol):

    MAGIC_BYTE: Final[int] = 0xEB

    transport: ReaderThread
    log_table: dict[str, str]

    start_byte_received: bool
    data_remaining: int | None
    buffer: bytearray

    last_cmd_id: int
    outstanding_cmds: dict[int, callable]

    def __init__(self):
        self.transport = None

        self.load_log_table()
        self.start_byte_received = False
        self.data_remaining = None
        self.buffer = bytearray()
        self.last_cmd_id = 0
        self.outstanding_cmds = {}

    def load_log_table(self) -> None:
        try:
            with open(LOG_TABLE_FILE, "r") as file:
                self.log_table = json.load(file)
        except FileNotFoundError:
            logger.warning(f"Log table file not found: {LOG_TABLE_FILE}")
            self.log_table = {}

    def connection_made(self, transport: ReaderThread) -> None:
        logger.info("Connected to device.")
        self.transport = transport
        self.transport.serial.reset_input_buffer()

    def connection_lost(self, exc) -> None:
        self.transport = None
        logger.info("Disconnected from device.")
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
        entry = self.log_table.get(str(log.hash))
        if entry is None:
            logger.warning(f"Unknown hash: {log.hash}")
            return

        args = self.parse_log_args(log.args)
        logger.log(logging.getLevelName(level), entry["msg"], *args)

    def handle_command_response(self, cmd: command_pb2.Command) -> None:
        if cmd.cmd_id in self.outstanding_cmds:
            callback = self.outstanding_cmds.pop(cmd.cmd_id)
            if callback:
                callback(cmd)

        else:
            logger.warning(f"Unknown command ID: {cmd.cmd_id}")

    def handle_packet(self, packet: bytes) -> None:
        try:
            stream = stream_pb2.StreamPacket()
            stream.ParseFromString(packet)
            if stream.HasField("log"):
                self.handle_log(stream.log)
            elif stream.HasField("cmd_response"):
                self.handle_command_response(stream.cmd_response)

        except Exception as e:
            logger.exception(f"Error: {e}")

    def _send_command(self, cmd, success_msg=None, error_msg=None):
        """
        Common method to send commands to the device.

        Args:
            cmd: The command protobuf object to send
            success_msg: Optional message to log on success
            error_msg: Optional message to log on error

        Returns:
            bool: True if command was sent successfully, False otherwise
        """
        cmd_bytes = cmd.SerializeToString()

        try:
            payload = struct.pack("<BB", self.MAGIC_BYTE, len(cmd_bytes))
            payload += cmd_bytes

            bytes_written = self.transport.write(payload)
            if bytes_written != len(payload):
                logger.error("Failed to write command to device.")
                del self.outstanding_cmds[cmd.cmd_id]
                return False

            if success_msg:
                logger.info(success_msg)
            return True

        except Exception as e:
            logger.exception(f"Error writing to device: {e}")
            del self.outstanding_cmds[cmd.cmd_id]
            return False

    def _create_command(self, callback=None):
        """
        Create a new command with incremented ID and register callback.

        Args:
            callback: Function to call when response is received

        Returns:
            command_pb2.Command: New command object with ID assigned
        """
        cmd = command_pb2.Command()
        cmd.cmd_id = self.last_cmd_id + 1
        self.last_cmd_id = cmd.cmd_id

        if callback:
            self.outstanding_cmds[cmd.cmd_id] = callback

        return cmd

    def configure_recording(self, enable: bool | None = None) -> None:
        """Configure audio recording on the device.

        Args:
            enable: Enable or disable recording (optional)

        """

        def cmd_cb(resp):
            if resp.status == command_pb2.CommandStatus.SUCCESS:
                logger.info("Recording configured successfully.")
            else:
                logger.error(f"Failed to configure recording: {resp.status}")

        cmd = self._create_command(cmd_cb)

        if enable is not None:
            cmd.configure_recording.enable = enable

        self._send_command(cmd)

    def configure_playback(
        self,
        enable: bool | None = None,
        volume: float | None = None,
    ) -> None:
        """
        Configure audio playback on the device.

        Args:
            enable: Enable or disable playback (optional)
            volume: Volume level between 0.0 and 1.0 (optional)
        """

        def cmd_cb(resp):
            if resp.status == command_pb2.CommandStatus.SUCCESS:
                logger.info("Playback configured successfully.")
            else:
                logger.error(f"Failed to configure playback: {resp.status}")

        cmd = self._create_command(cmd_cb)

        if enable is not None:
            cmd.configure_playback.enable = enable

        if volume is not None:
            cmd.configure_playback.volume = max(0.0, min(1.0, volume))

        self._send_command(cmd)

    def set_volume(self, volume: float) -> None:
        """
        Set the audio volume.

        Args:
            volume: Volume level between 0.0 and 1.0
        """
        # We use configure_playback with just the volume parameter
        self.configure_playback(volume=volume)

    def reset_device(self) -> None:
        """Send a reset command to the device."""

        def cmd_cb(resp):
            if resp.status == command_pb2.CommandStatus.SUCCESS:
                logger.info("Device reset command sent successfully.")
            else:
                logger.error(f"Failed to reset device: {resp.status}")

        cmd = self._create_command(cmd_cb)
        cmd.reset.SetInParent()  # Initialize the reset message

        self._send_command(
            cmd,
            success_msg="Reset command sent. Device will reset momentarily.",
        )


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
