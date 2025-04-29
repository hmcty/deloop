"""Primary user interface for Mk0."""

import argparse
import logging
import threading
from contextlib import ExitStack
from logging.handlers import QueueHandler
from queue import Queue

import cmd2
import pyinotify
from deloop_mk0.uart_stream import (LOG_TABLE_FILE, Mk0Stream, add_uart_args,
                                    open_uart_stream)
from deloop_mk0.utils import ColoredFormatter

logger = logging.getLogger()  # Root logger
logger.setLevel(logging.DEBUG)

log_queue = Queue()
handler = QueueHandler(log_queue)
logger.addHandler(handler)

formatter = ColoredFormatter("%(asctime)s | %(levelname)8s | %(message)s")
handler.setFormatter(formatter)


class OnWriteHandler(pyinotify.ProcessEvent):

    def my_init(self, stream: Mk0Stream) -> None:
        self._stream = stream

    def process_IN_CLOSE_WRITE(self, event) -> None:
        if event.pathname != str(LOG_TABLE_FILE):
            return

        logger.info("Sources changed. Reloading...")
        self._stream.load_log_table()


class Mk0Cmd(cmd2.Cmd):

    def __init__(self, stream: Mk0Stream, log_queue: Queue):
        super().__init__(allow_cli_args=False)
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

    def do_enable_recording(self, _) -> None:
        """Enable recording of audio on the device."""

        self._stream.configure_recording(enable=True)

    def do_disable_recording(self, _) -> None:
        """Disable recording of audio on the device."""

        self._stream.configure_recording(enable=False)

    def do_enable_playback(self, _) -> None:
        """Enable audio playback on the device."""

        self._stream.configure_playback(enable=True)

    def do_disable_playback(self, _) -> None:
        """Disable audio playback on the device."""

        self._stream.configure_playback(enable=False)

    def do_set_volume(self, arg) -> None:
        """
        Set the audio volume (0.0 to 1.0).

        Usage: set_volume 0.5  # Set to 50%
        """
        try:
            volume = float(arg)
            volume = max(0.0, min(1.0, volume))
            self._stream.set_volume(volume)
            print(f"Volume set to {int(volume * 100)}%")

        except ValueError:
            print("Error: Volume must be a number between 0.0 and 1.0")

    def do_reset(self, _) -> None:
        """Reset the device (performs a soft reset)."""

        print(
            "Are you sure you want to reset the device? (y/n) ",
            end="",
            flush=True,
        )
        response = input().lower()
        if response in ['y', 'yes']:
            self._stream.reset_device()
        else:
            print("Reset cancelled.")

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
    parser.add_argument(
        "--hotreload",
        action="store_true",
        help="Enable hot reloading of log table files.",
    )
    args = parser.parse_args()

    with ExitStack() as stack, open_uart_stream(args) as mk0_stream:
        if args.hotreload:
            wm = pyinotify.WatchManager()
            handler = OnWriteHandler(stream=mk0_stream)
            notifier = pyinotify.ThreadedNotifier(wm, default_proc_fun=handler)
            wm.add_watch(str(LOG_TABLE_FILE), pyinotify.ALL_EVENTS)
            notifier.start()
            stack.callback(notifier.stop)

        cmd = Mk0Cmd(mk0_stream, log_queue)
        cmd.cmdloop()


if __name__ == "__main__":
    main()
