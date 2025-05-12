#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PYTHON_PATH="$SCRIPT_DIR/../python/"

cp $1/*_pb2.py $PYTHON_PATH
cp $1/log_table.json $PYTHON_PATH/deloop_mk0
