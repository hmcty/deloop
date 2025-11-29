#!/bin/bash

make || { echo "Build failed"; exit 1; }
echo -ne "@RST\r\n" > /dev/serial/by-id/*Daisy_Seed*
sleep 1.5
make dfu_app
