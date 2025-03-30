#!/usr/bin/env python3
"""Parses logging calls into a table of unique identifiers.

Example usage:
```sh
create_log_table.py --output table.json --macro DELOOP_LOG src1.c src2.c ...
```

This script expects the logging macro to follow a specific format:
```c
#define <MACRO>(MSG, ...)
```

You can pass multiple macro names.

"""

import argparse
import json
import os
import re

MACRO_PATTERN = r'({MACROS})\s*\(\s*(".*?(?<!\\)")'


def fnv1a_64(s: str) -> int:
    FNV_OFFSET = 0xcbf29ce484222325
    FNV_PRIME = 0x100000001b3
    hash = FNV_OFFSET
    for c in s.encode('utf-8'):
        hash ^= c
        hash = (hash * FNV_PRIME) & (2**64 - 1)
    return hash


def main(args: argparse.Namespace) -> None:
    if not args.macro:
        print("No macros provided.")
        return

    elif not args.source_files:
        print("No source files provided.")
        return

    macro_re = re.compile(
        MACRO_PATTERN.format(MACROS="|".join(args.macro)),
        re.VERBOSE | re.DOTALL,
    )

    log_table = {}
    for source_file in args.source_files:
        if not os.path.isfile(source_file):
            print(f"File not found: {source_file}")
            continue

        with open(source_file, "r") as file:
            content = file.read()
            matches = macro_re.findall(content)
            for (_, match) in matches:
                msg = match.strip('"')
                log_table[fnv1a_64(msg)] = msg

    with open(args.output, "w") as output_file:
        json.dump(log_table, output_file, indent=2)

    print(f"Wrote {len(log_table)} log calls to {args.output}")

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument(
        "--output",
        type=str,
        required=True,
        help="Path to the output JSON file.",
    )
    arg_parser.add_argument(
        "--macro",
        type=str,
        nargs="+",
        help="List of logging macros to parse.",
    )
    arg_parser.add_argument(
        "--source_files",
        nargs="+",
        help="List of source files to parse for logging calls.",
    )
    main(arg_parser.parse_args())
