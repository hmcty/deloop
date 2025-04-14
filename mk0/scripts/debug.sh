#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "$1" ]; then
    ELF_PATH="$SCRIPT_DIR/../result/bin/deloop_mk0.elf"
else
    ELF_PATH="$1"
fi

if [ ! -f "$ELF_PATH" ]; then
    echo "Error: $ELF_PATH not found. Please run `nix-build` first or pass-in."
    exit 1
elif ! command -v st-util &> /dev/null
then
    echo "st-util could not be found. Please install st-link."
    exit 1
elif ! command -v arm-none-eabi-gdb &> /dev/null
then
    echo "arm-none-eabi-gdb could not be found. Please install ARM toolchain."
    exit 1
fi

cleanup() {
  kill -HUP -"$st_util_pid" 2>/dev/null || true
}
trap cleanup EXIT

setsid st-util &
st_util_pid=$!

arm-none-eabi-gdb "$ELF_PATH" -x $SCRIPT_DIR/stm32f4.gdb
