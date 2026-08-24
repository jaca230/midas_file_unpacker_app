#!/bin/bash

set -e

SCRIPT_DIRECTORY="$(
    cd -P "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd
)"
# shellcheck disable=SC1091
source "$SCRIPT_DIRECTORY/environment_paths.sh"

RECREATE=false
RUN_CHECK=true

while [[ $# -gt 0 ]]; do
    case "$1" in
        --recreate)
            RECREATE=true
            shift
            ;;
        --no-check)
            RUN_CHECK=false
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--recreate] [--no-check]"
            exit 0
            ;;
        *)
            echo "ERROR: unknown option '$1'" >&2
            exit 2
            ;;
    esac
done

bootstrap_micromamba() {
    if [ -x "$UNPACKER_MICROMAMBA" ]; then
        return
    fi

    local platform
    case "$(uname -s)-$(uname -m)" in
        Linux-x86_64) platform="linux-64" ;;
        Linux-aarch64|Linux-arm64) platform="linux-aarch64" ;;
        Darwin-x86_64) platform="osx-64" ;;
        Darwin-arm64) platform="osx-arm64" ;;
        *)
            echo "ERROR: unsupported platform $(uname -s)-$(uname -m)" >&2
            exit 1
            ;;
    esac

    local temporary_directory
    temporary_directory="$(mktemp -d)"
    trap 'rm -rf "$temporary_directory"' EXIT

    echo "Downloading micromamba for $platform..."
    curl --fail --location --silent --show-error \
        "https://micro.mamba.pm/api/micromamba/$platform/latest" |
        tar -xj -C "$temporary_directory" bin/micromamba
    mkdir -p "$(dirname "$UNPACKER_MICROMAMBA")"
    install -m 0755 "$temporary_directory/bin/micromamba" \
        "$UNPACKER_MICROMAMBA"
    rm -rf "$temporary_directory"
    trap - EXIT
}

bootstrap_micromamba
export MAMBA_ROOT_PREFIX="$UNPACKER_MAMBA_ROOT_PREFIX"

if [ "$RECREATE" = true ] && [ -d "$UNPACKER_ENV_PREFIX/conda-meta" ]; then
    "$UNPACKER_MICROMAMBA" env remove --yes --name "$UNPACKER_ENV_NAME"
fi

if [ -d "$UNPACKER_ENV_PREFIX/conda-meta" ]; then
    echo "Updating environment '$UNPACKER_ENV_NAME'..."
    "$UNPACKER_MICROMAMBA" env update --yes --name "$UNPACKER_ENV_NAME" \
        --file "$UNPACKER_ENV_FILE" --prune
else
    echo "Creating environment '$UNPACKER_ENV_NAME'..."
    "$UNPACKER_MICROMAMBA" create --yes --name "$UNPACKER_ENV_NAME" \
        --file "$UNPACKER_ENV_FILE"
fi

if [ "$RUN_CHECK" = true ]; then
    "$UNPACKER_MICROMAMBA" run --name "$UNPACKER_ENV_NAME" bash -c \
        "source '$UNPACKER_PROJECT_ROOT/scripts/setup_env.sh' &&
         '$SCRIPT_DIRECTORY/check_env.sh'"
fi

echo
echo "Environment ready. Activate it with:"
echo "  source scripts/setup_env.sh"
