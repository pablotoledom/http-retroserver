#!/bin/bash
source "$(dirname "$0")/show/welcome"

INSTALL_LIB="/usr/local/lib/retroserver"
INSTALL_BIN="/usr/local/bin/retroserver"

# ── 1. Compile if binary is missing ──────────────────────────────────────────
source "$(dirname "$0")/show/divbar"
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [ ! -f "$SCRIPT_DIR/bin-linux/retroserver" ]; then
    echo "1- Binary not found. Compiling for Linux..."
    "$SCRIPT_DIR/scripts/compile_linux.sh"
else
    echo "1- Binary already compiled, skipping build."
fi

# ── 2. Install binary and configs ────────────────────────────────────────────
source "$(dirname "$0")/show/divbar"
echo "2- Installing binary and configs to $INSTALL_LIB ..."

mkdir -p "$INSTALL_LIB"
cp "$SCRIPT_DIR/bin-linux/retroserver" "$INSTALL_LIB/retroserver"
cp -r "$SCRIPT_DIR/configs" "$INSTALL_LIB/configs"
chmod +x "$INSTALL_LIB/retroserver"

# ── 3. Create wrapper script in PATH ─────────────────────────────────────────
source "$(dirname "$0")/show/divbar"
echo "3- Creating command wrapper at $INSTALL_BIN ..."

cat > "$INSTALL_BIN" << 'EOF'
#!/bin/bash
INSTALL_LIB="/usr/local/lib/retroserver"
SERVE_DIR="${1:-.}"

if [ ! -d "$SERVE_DIR" ]; then
    echo "Error: directory '$SERVE_DIR' does not exist."
    exit 1
fi

SERVE_REALPATH="$(realpath "$SERVE_DIR")"

cd "$INSTALL_LIB"
exec ./retroserver "$SERVE_REALPATH"
EOF

chmod +x "$INSTALL_BIN"

# ── Done ─────────────────────────────────────────────────────────────────────
source "$(dirname "$0")/show/divbar"
echo "Installed!"
echo "  Binary : $INSTALL_LIB/retroserver"
echo "  Config : $INSTALL_LIB/configs/config.txt"
echo "  Command: $INSTALL_BIN"
echo ""
echo "Usage from anywhere:"
echo "  retroserver .            # serve current directory"
echo "  retroserver /some/path   # serve a specific directory"
