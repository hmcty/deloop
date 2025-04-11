#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELF_PATH="$SCRIPT_DIR/../result/bin/deloop_mk0.elf"
if [ ! -f "$ELF_PATH" ]; then
    echo "Error: $ELF_PATH not found. Please run `nix-build` first."
    exit 1
fi

st-util &
arm-none-eabi-gdb "$ELF_PATH" -x $SCRIPT_DIR/stm32f4.gdb
