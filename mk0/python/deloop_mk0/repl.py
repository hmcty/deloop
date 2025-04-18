"""Primary user interface for Mk0."""

import argparse
import logging
import threading
from logging.handlers import QueueHandler
from queue import Queue

import cmd2
from deloop_mk0.uart_stream import Mk0Stream, add_uart_args, open_uart_stream
from deloop_mk0.utils import ColoredFormatter

logger = logging.getLogger()  # Root logger
logger.setLevel(logging.DEBUG)

log_queue = Queue()
handler = QueueHandler(log_queue)
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

        # Delete lots of the default commands
        del cmd2.Cmd.do_edit
        del cmd2.Cmd.do_shell
        del cmd2.Cmd.do_alias
        del cmd2.Cmd.do_macro
        del cmd2.Cmd.do_run_pyscript
        del cmd2.Cmd.do_run_script
        del cmd2.Cmd.do_shortcuts
        del cmd2.Cmd.do_set

    def do_enable_logs(self, _) -> None:
        """Enable printing of logs from the device."""

        if self._log_thread.is_alive():
            print("Logs are already enabled.")
            return

        else:
            self._stop_event.clear()
            self._log_thread = threading.Thread(target=self._log_thread_loop)
            self._log_thread.start()

    def do_disable_logs(self, _) -> None:
        """Enable printing of logs from the device."""

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

                finally:
                    self.terminal_lock.release()

            self._stop_event.wait(0.1)


def main() -> None:
    parser = argparse.ArgumentParser()
    add_uart_args(parser)
    args = parser.parse_args()

    with open_uart_stream(args) as mk0_stream:
        cmd = Mk0Cmd(mk0_stream, log_queue)
        cmd.cmdloop()


if __name__ == "__main__":
    main()
