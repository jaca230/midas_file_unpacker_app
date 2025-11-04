#!/bin/bash

# Save original working directory
ORIG_DIR=$(pwd)

# Get the absolute path of the script directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE_DIR="$SCRIPT_DIR/.."
EXECUTABLE="$BASE_DIR/build/bin/unpacker"

# Variables for options
PRELOAD_LIBS=""
DEBUG=false
VALGRIND=false
PROFILE=""
EXE_ARGS=()

# Help message
show_help() {
    echo "Usage: ./run.sh [OPTIONS] -- <input_midas_file> [max_events|--options]"
    echo
    echo "Options:"
    echo "  -h, --help           Display this help message"
    echo "  -d, --debug          Run with gdb for debugging"
    echo "  -v, --valgrind       Run with valgrind for memory analysis"
    echo "  --preload <libs>     Comma-separated list of library paths to LD_PRELOAD"
    echo "  --profile <name>     Select pipeline profile (forwarded to unpacker unless provided after '--')"
    echo
    echo "Arguments after '--' must include:"
    echo "  input_midas_file      Path to input MIDAS file"
    echo "  Additional options accepted by the unpacker (e.g. --profile, --max-events)"
    echo
    echo "Example:"
    echo "  ./run.sh --profile HDSoC -- midas_file.mid.lz4 --max-events 100"
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help; exit 0 ;;
        -d|--debug) DEBUG=true; shift ;;
        -v|--valgrind) VALGRIND=true; shift ;;
        --preload)
            if [[ -n "$2" && "$2" != -* ]]; then
                PRELOAD_LIBS="${2//,/':' }"
                shift 2
            else
                echo "[run.sh, ERROR] --preload requires a comma-separated list of paths"
                exit 1
            fi
            ;;
        --profile)
            if [[ -n "$2" && "$2" != -* ]]; then
                PROFILE="$2"
                shift 2
            else
                echo "[run.sh, ERROR] --profile requires a profile name"
                exit 1
            fi
            ;;
        --) shift; EXE_ARGS+=("$@"); break ;;
        *) echo "[run.sh, ERROR] Unknown option: $1"; show_help; exit 1 ;;
    esac
done

# Ensure an input file is provided among forwarded arguments
if [ "${#EXE_ARGS[@]}" -lt 1 ]; then
    echo "[run.sh, ERROR] Missing required argument: input_midas_file"
    show_help
    exit 1
fi

INPUT_FILE=""
for arg in "${EXE_ARGS[@]}"; do
    if [[ "$arg" != -* ]]; then
        INPUT_FILE="$arg"
        break
    fi
done

if [[ -z "$INPUT_FILE" ]]; then
    echo "[run.sh, ERROR] Could not determine input MIDAS file from arguments"
    show_help
    exit 1
fi

# Check executable presence
if [ ! -f "$EXECUTABLE" ]; then
    echo "[run.sh, ERROR] Executable not found at: $EXECUTABLE"
    echo "Try running './build.sh' first."
    exit 1
fi

# Export LD_PRELOAD if any preload libs specified
if [[ -n "$PRELOAD_LIBS" ]]; then
    export LD_PRELOAD="$PRELOAD_LIBS${LD_PRELOAD:+:$LD_PRELOAD}"
fi

# Change to base dir
cd "$BASE_DIR" || {
    echo "[run.sh, ERROR] Failed to change directory to $BASE_DIR"
    exit 1
}

# Build command line for executable
CMD=("$EXECUTABLE")
if [[ -n "$PROFILE" ]]; then
    profile_already_set=false
    for ((i=0; i<${#EXE_ARGS[@]}; ++i)); do
        if [[ "${EXE_ARGS[i]}" == "--profile" ]]; then
            profile_already_set=true
            break
        fi
    done
    if [ "$profile_already_set" = false ]; then
        CMD+=("--profile" "$PROFILE")
    fi
fi

CMD+=("${EXE_ARGS[@]}")

# Run executable
echo "[run.sh, INFO] Running analysis_pipeline..."
if [ "$DEBUG" = true ]; then
    echo "[run.sh, INFO] Running with gdb..."
    gdb --args "${CMD[@]}"
elif [ "$VALGRIND" = true ]; then
    echo "[run.sh, INFO] Running with valgrind..."
    valgrind --leak-check=full --track-origins=yes "${CMD[@]}"
else
    "${CMD[@]}"
fi

# Return to original directory
cd "$ORIG_DIR"
