#!/usr/bin/env bash

if [ -z "$1" ]; then
  SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  BIN_PATH="$SCRIPT_DIR/../result/bin/deloop_mk0.bin"
  if [ ! -f "$BIN_PATH" ]; then
    echo "Error: $BIN_PATH not found. Please run `nix-build` first."
    exit 1
  fi
else
  BIN_PATH="$1"
  if [ ! -f "$BIN_PATH" ]; then
    echo "Error: $BIN_PATH not found."
    exit 1
  fi
fi

st-flash write "$BIN_PATH" 0x08000000
