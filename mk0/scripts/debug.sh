#!/usr/bin/env nix-shell
#! nix-shell -i bash ../shell.nix

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

st-util &
arm-none-eabi-gdb "$DELOOP_MK0_PATH/bin/deloop_mk0.elf" -x $SCRIPT_DIR/stm32f4.gdb
