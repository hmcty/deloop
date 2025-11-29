#!/usr/bin/python3
import argparse
import queue
import threading
import time

import serial
from IPython import embed


class SerialHandler(threading.Thread):

  def __init__(self, port: str, baudrate: int):
    super().__init__()
    self.dev = serial.Serial(port, baudrate, timeout=1)
    self.should_exit = threading.Event()
    self.cmd_queue = queue.Queue()

  def cmd_reset(self):
    self.cmd_queue.put("@RST\r\n")

  def cmd_version(self):
    self.cmd_queue.put("@VER\r\n")

  def cmd_tree(self):
    self.cmd_queue.put("@TREE\r\n")

  def cmd_raw(self, cmd: str):
    self.cmd_queue.put(cmd + "\r\n")

  def run(self):
    while not self.should_exit.is_set():
      try:
        if not self.dev.is_open:
          self.dev.open()
          if self.dev.is_open:
            print(f"Re-connected to device.")

        if self.dev.in_waiting > 0:
          print(self.dev.read(self.dev.in_waiting).decode(), end='')

      except (serial.SerialException, OSError):
        self.dev.close()
        time.sleep(0.5)
        print(".", end='', flush=True)
        continue

      while not self.cmd_queue.empty():
        cmd = self.cmd_queue.get()
        self.dev.write(cmd.encode())

  def __enter__(self):
    self.dev.__enter__()
    self.start()
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    self.should_exit.set()
    self.join()
    self.dev.__exit__(exc_type, exc_value, traceback)


def main(args):
  with SerialHandler(args.port, args.baudrate) as handler:
    ns = {
      "reset": handler.cmd_reset,
      "version": handler.cmd_version,
      "tree": handler.cmd_tree,
      "raw": handler.cmd_raw,
    }
    embed(colors="neutral", user_ns=ns)


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--port", type=str, required=True, help="Serial port to connect to")
  parser.add_argument("--baudrate", type=int, default=115200, help="Baud rate for the serial connection")
  args = parser.parse_args()
  main(args)
