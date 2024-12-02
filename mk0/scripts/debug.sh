#!/usr/bin/env nix-shell
#! nix-shell -i fish
#! nix-shell -p fish stlink gcc-arm-embedded-9

st-util &
arm-none-eabi-gdb result/bin/deloop_mk0.elf -x scripts/stm32f4.gdb

