#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "ERROR: source this script: source scripts/deactivate_env.sh" >&2
    exit 1
fi

if declare -F micromamba >/dev/null 2>&1; then
    micromamba deactivate
fi

if [ "${UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH_WAS_SET:-0}" = 1 ]; then
    export CMAKE_PREFIX_PATH="$UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH"
else
    unset CMAKE_PREFIX_PATH
fi

if [ -n "${UNPACKER_PREVIOUS_PATH:-}" ]; then
    export PATH="$UNPACKER_PREVIOUS_PATH"
    hash -r
fi

unset UNPACKER_APP_ROOT
unset UNPACKER_PREVIOUS_PATH
unset UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH
unset UNPACKER_PREVIOUS_CMAKE_PREFIX_PATH_WAS_SET
unset UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE
unset UNPACKER_STAGES_SAMPIC_SOURCE
unset UNPACKER_BUILD_JOBS

echo "Deactivated the MIDAS unpacker environment."
