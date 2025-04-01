#!/usr/bin/env nix-shell
#! nix-shell -i bash ../shell.nix

st-flash write "$DELOOP_MK0_PATH/bin/deloop_mk0.bin" 0x08000000
