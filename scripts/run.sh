#!/bin/bash

SERVE_DIR="${1:-.}"

if [ ! -d "$SERVE_DIR" ]; then
    echo "Error: directory '$SERVE_DIR' does not exist."
    exit 1
fi

# Make sure the binary exists
if [ ! -f "./bin/retroserver" ]; then
    echo "Binary not found. Run 'compiledebug' or 'compile' first."
    exit 1
fi

SERVE_REALPATH="$(realpath "$SERVE_DIR")"

echo "Serving: $SERVE_REALPATH"
echo "Press Ctrl+C to stop."
echo ''

cd bin
./retroserver "$SERVE_REALPATH"
