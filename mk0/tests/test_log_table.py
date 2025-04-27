import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock
from unittest.mock import mock_open, patch

sys.path.append(str(Path(__file__).parent.parent / "scripts"))
import create_log_table  # noqa: E402


class TestCreateLogTable(unittest.TestCase):

    @patch("builtins.open", new_callable=mock_open, read_data="")
    def test_no_macros_provided(self, mock_file):
        args = mock.Mock()
        args.macro = []
        args.source_files = ["dummy.c"]
        args.output = "output.json"

        with patch("builtins.print") as mock_print:
            create_log_table.main(args)
            mock_print.assert_called_with("No macros provided.")

    @patch("builtins.open", new_callable=mock_open, read_data="")
    def test_no_source_files_provided(self, mock_file):
        args = mock.Mock()
        args.macro = ["DELOOP_LOG"]
        args.source_files = []
        args.output = "output.json"

        with patch("builtins.print") as mock_print:
            create_log_table.main(args)
            mock_print.assert_called_with("No source files provided.")

    def test_basic(self):
        self._test_log_parsing_and_output(
            content="""
            #include <stdio.h>
            #define DELOOP_LOG(MSG, ...) printf(MSG, __VA_ARGS__)
            DELOOP_LOG("Log message here");
            """,
            macros=["DELOOP_LOG"],
            expected_logs=["Log message here"],
        )

    def test_many_args(self):
        self._test_log_parsing_and_output(
            content="""
            DELOOP_LOG(
                "Log message %d with %s!",
                42,  12 + 6,
                10233213 + 9);
            """,
            macros=["DELOOP_LOG"],
            expected_logs=["Log message %d with %s!"],
        )

    def test_multiple_macros(self):
        self._test_log_parsing_and_output(
            content="""
            #define DELOOP_LOG(MSG, ...) printf(MSG, __VA_ARGS__)
            #define OTHER_LOG(MSG, ...) printf(MSG, __VA_ARGS__)
            DELOOP_LOG("Log message here");
            OTHER_LOG("Another log message here");
            """,
            macros=["DELOOP_LOG", "OTHER_LOG"],
            expected_logs=["Log message here", "Another log message here"],
        )

    def _test_log_parsing_and_output(
        self,
        content: str,
        macros: list[str],
        expected_logs: list[str],
    ) -> None:
        with tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".c",
            delete=False,
        ) as tmp:
            tmp.write(content)
            tmp_path = tmp.name

        with tempfile.NamedTemporaryFile(
            mode="r+",
            suffix=".json",
            delete=False,
        ) as output_tmp:
            args = mock.Mock()
            args.macro = macros
            args.source_files = [tmp_path]
            args.source_version = "1.0.0"
            args.output = output_tmp.name

            with patch("builtins.print") as mock_print:
                create_log_table.main(args)
                mock_print.assert_any_call(
                    f"Wrote {len(expected_logs)} log "
                    f"calls to {output_tmp.name}"
                )

            result = json.load(output_tmp)
            for k, v in result.items():
                self.assertIn(v["msg"], expected_logs)
                self.assertEqual(int(k), create_log_table.fnv1a_64(v["msg"]))


if __name__ == '__main__':
    unittest.main()
