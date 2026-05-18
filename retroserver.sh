#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Use: $0 <action> [args...]"
    echo ""
    echo "  compile        Build for production (Release)"
    echo "  compiledebug   Build with debug symbols"
    echo "  run [dir]      Run the server (default dir: current directory)"
    echo "  install        Install as systemd service"
    echo "  uninstall      Remove systemd service"
    echo ""
    exit 1
fi

while [ $# -gt 0 ]; do
    action="$1"
    shift
    case "$action" in
        compile)
            echo "Compiling for production..."
            ./scripts/compile.sh
            ;;
        compiledebug)
            echo "Compiling for debug..."
            ./scripts/compile_debug.sh
            ;;
        run)
            ./scripts/run.sh "$1"
            shift
            ;;
        install)
            echo "Installing..."
            sudo ./scripts/install.sh
            ;;
        uninstall)
            echo "Uninstalling..."
            sudo ./scripts/uninstall.sh
            ;;
        *)
            echo "Unknown action: $action"
            echo "Valid actions: compile, compiledebug, run, install, uninstall"
            ;;
    esac
done
