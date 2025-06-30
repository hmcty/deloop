#!/usr/bin/env bash

cross build --target=armv7-unknown-linux-gnueabihf --no-default-features --features=embedded
if [ $? -ne 0 ]; then
    echo "Build failed. Please check the output for errors."
    exit 1
fi

if [ -z "$1" ]; then
    ELF_PATH="./target/armv7-unknown-linux-gnueabihf/debug/deloop"
else
    ELF_PATH="$1"
fi

scp "$ELF_PATH" pi@raspberrypi.local:deloop_bin
