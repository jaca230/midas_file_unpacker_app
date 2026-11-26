#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "ERROR: source this script: source scripts/setup_env.sh" >&2
    exit 1
fi

SCRIPT_DIRECTORY="$(
    cd -P "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd
)"
# shellcheck disable=SC1091
source "$SCRIPT_DIRECTORY/environment/environment_paths.sh"

if [ "${CONDA_PREFIX:-}" = "$UNPACKER_ENV_PREFIX" ]; then
    echo "Environment '$UNPACKER_ENV_NAME' is already active."
    return 0
fi

if [ ! -x "$UNPACKER_MICROMAMBA" ] ||
   [ ! -d "$UNPACKER_ENV_PREFIX/conda-meta" ]; then
    echo "Environment '$UNPACKER_ENV_NAME' is missing; creating it now."
    "$SCRIPT_DIRECTORY/environment/create_env.sh" --no-check || return 1
fi

export UNPACKER_PREVIOUS_PATH="$PATH"
if [ "${CMAKE_PREFIX_PATH+x}" = x ]; then
    export UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"
    export UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH_WAS_SET=1
else
    unset UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH
    export UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH_WAS_SET=0
fi

export MAMBA_ROOT_PREFIX="$UNPACKER_MAMBA_ROOT_PREFIX"
eval "$("$UNPACKER_MICROMAMBA" shell hook --shell bash)"
micromamba activate "$UNPACKER_ENV_NAME"

if [ -f "$UNPACKER_PROJECT_ROOT/.env" ]; then
    set -a
    # shellcheck disable=SC1091
    source "$UNPACKER_PROJECT_ROOT/.env"
    set +a
fi

export UNPACKER_APP_ROOT="$UNPACKER_PROJECT_ROOT"
export CMAKE_PREFIX_PATH="$CONDA_PREFIX${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"

echo "Activated $UNPACKER_ENV_NAME"
echo "  CONDA_PREFIX=$CONDA_PREFIX"
echo "  ROOT=$(command -v root-config 2>/dev/null || echo missing)"
