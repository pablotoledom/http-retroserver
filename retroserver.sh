#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Use: $0 <action> [args...]"
    echo ""
    echo "  linux          Build for Linux (Release)"
    echo "  macos          Build for macOS (run this on a Mac)"
    echo "  win32          Cross-compile for Windows 32-bit (Win95 to Win11)"
    echo "  debug          Build for Linux with debug symbols"
    echo "  run [dir]      Run the server (default dir: current directory)"
    echo "  install-service Install as systemd service"
    echo "  install-global Install binary globally (retroserver available from anywhere)"
    echo "  uninstall      Remove systemd service"
    echo ""
    exit 1
fi

while [ $# -gt 0 ]; do
    action="$1"
    shift
    case "$action" in
        linux)
            ./scripts/compile_linux.sh
            ;;
        macos)
            ./scripts/compile_macos.sh
            ;;
        win32)
            ./scripts/compile_win32.sh
            ;;
        debug)
            ./scripts/compile_debug.sh
            ;;
        run)
            ./scripts/run.sh "$1"
            shift
            ;;
        install-service)
            echo "Installing..."
            sudo ./scripts/install_service.sh
            ;;
        install-global)
            echo "Installing globally..."
            sudo ./scripts/install_global.sh
            ;;
        uninstall)
            echo "Uninstalling..."
            sudo ./scripts/uninstall.sh
            ;;
        *)
            echo "Unknown action: $action"
            echo "Valid actions: linux, macos, win32, debug, run, install-service, install-global, uninstall"
            ;;
    esac
done
