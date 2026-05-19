#!/bin/bash

SERVE_DIR="${1:-.}"

if [ ! -d "$SERVE_DIR" ]; then
    echo "Error: directory '$SERVE_DIR' does not exist."
    exit 1
fi

# Make sure the binary exists
if [ ! -f "./bin-linux/retroserver" ]; then
    echo "Binary not found. Run './retroserver.sh linux' first."
    exit 1
fi

SERVE_REALPATH="$(realpath "$SERVE_DIR")"

cd bin-linux
./retroserver "$SERVE_REALPATH"
