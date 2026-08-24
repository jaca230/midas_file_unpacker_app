#!/bin/bash

set -euo pipefail

# Resolve absolute paths
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE_DIR=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$BASE_DIR/build"
CLEANUP_SCRIPT="$SCRIPT_DIR/cleanup.sh"

if [ -f "$BASE_DIR/.env" ]; then
    set -a
    # shellcheck disable=SC1091
    source "$BASE_DIR/.env"
    set +a
fi

# ROOT dictionary generation is memory-heavy. Two concurrent jobs is a safer
# default for development machines; callers can still override it with -j.
OVERWRITE=false
BUILD_JOBS="${UNPACKER_BUILD_JOBS:-2}"

# Help message
show_help() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo
    echo "Options:"
    echo "  -o, --overwrite           Remove existing build directory before building"
    echo "  -j, --jobs <number>       Specify parallel jobs (default: $BUILD_JOBS)"
    echo "  -h, --help                Display this help message"
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -o|--overwrite)
            OVERWRITE=true
            shift
            ;;
        -j|--jobs)
            if [[ ! "${2:-}" =~ ^[1-9][0-9]*$ ]]; then
                echo "[build.sh, ERROR] --jobs requires a positive integer"
                exit 2
            fi
            BUILD_JOBS="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "[build.sh, ERROR] Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Optionally clean build
if [ "$OVERWRITE" = true ]; then
    echo "[build.sh] Cleaning previous build with: $CLEANUP_SCRIPT"
    "$CLEANUP_SCRIPT"
fi

# Create and enter build directory
mkdir -p "$BUILD_DIR"

# Run CMake and the selected generator's build command.
echo "[build.sh] Running cmake in: $BUILD_DIR"
CMAKE_TOOLCHAIN_ARGS=()
if command -v root-config >/dev/null 2>&1; then
    ROOT_CXX="$(root-config --cxx)"
    if command -v "$ROOT_CXX" >/dev/null 2>&1; then
        CMAKE_TOOLCHAIN_ARGS+=(
            "-DCMAKE_CXX_COMPILER=$(command -v "$ROOT_CXX")"
        )
        echo "[build.sh] Using ROOT toolchain: $ROOT_CXX"
    fi
fi
if [ -n "${UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE:-}" ]; then
    CMAKE_TOOLCHAIN_ARGS+=(
        "-DUNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE=$UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE"
    )
fi
if [ -n "${UNPACKER_STAGES_SAMPIC_SOURCE:-}" ]; then
    CMAKE_TOOLCHAIN_ARGS+=(
        "-DUNPACKER_STAGES_SAMPIC_SOURCE=$UNPACKER_STAGES_SAMPIC_SOURCE"
    )
fi
cmake -S "$BASE_DIR" -B "$BUILD_DIR" "${CMAKE_TOOLCHAIN_ARGS[@]}"

echo "[build.sh] Building with $BUILD_JOBS parallel jobs"
cmake --build "$BUILD_DIR" --parallel "$BUILD_JOBS"

echo "[build.sh] Build complete."
echo "[build.sh] Executables are in: $BUILD_DIR/bin/"
echo "[build.sh] Libraries are in: $BUILD_DIR/lib/"
