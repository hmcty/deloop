"""Primary user interface for Mk0."""

import argparse
import logging
import threading
from queue import Queue

import cmd2
from uart_stream import Mk0Stream, add_uart_args, open_uart_stream
from utils import ColoredFormatter

logger = logging.getLogger()  # Root logger
logger.setLevel(logging.DEBUG)

log_queue = Queue()
handler = logging.handlers.QueueHandler(log_queue)
logger.addHandler(handler)

formatter = ColoredFormatter("%(asctime)s | %(levelname)8s | %(message)s")
handler.setFormatter(formatter)


class Mk0Cmd(cmd2.Cmd):

    def __init__(self, stream: Mk0Stream, log_queue: Queue):
        super().__init__()
        self.prompt = "Mk0> "
        self.allow_style = cmd2.ansi.AllowStyle.TERMINAL

        self._stream = stream
        self._log_queue = log_queue
        self._stop_event = threading.Event()
        self._log_thread = threading.Thread(target=self._log_thread_loop)

        self.register_preloop_hook(self._preloop_hook)
        self.register_postloop_hook(self._postloop_hook)

    def do_enable_logs(self, _) -> None:
        if self._log_thread.is_alive():
            print("Logs are already enabled.")
            return

        else:
            self._stop_event.clear()
            self._log_thread = threading.Thread(target=self._log_thread_loop)
            self._log_thread.start()

    def disable_logs(self, _) -> None:
        self._stop_event.set()
        if self._log_thread.is_alive():
            self._log_thread.join()

        else:
            print("Logs are already disabled.")

    def _preloop_hook(self) -> None:
        self._stop_event.clear()
        self._log_thread = threading.Thread(target=self._log_thread_loop)
        self._log_thread.start()

    def _postloop_hook(self) -> None:
        self._stop_event.set()
        if self._log_thread.is_alive():
            self._log_thread.join()

    def _log_thread_loop(self) -> None:
        while not self._stop_event.is_set():
            if self.terminal_lock.acquire(blocking=False):
                try:
                    if not self._log_queue.empty():
                        log_record = self._log_queue.get()
                        self.async_alert(log_record.getMessage())

                    elif self.need_prompt_refresh:
                        self.async_refresh_prompt()

                finally:
                    self.terminal_lock.release()

            self._stop_event.wait(0.1)


def main(args: argparse.Namespace) -> None:
    with open_uart_stream(args) as mk0_stream:
        cmd = Mk0Cmd(mk0_stream, log_queue)
        cmd.cmdloop()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    add_uart_args(parser)
    main(parser.parse_args())
