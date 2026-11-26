#!/bin/bash

set -e

failures=0

check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        printf '[OK]   %-12s %s\n' "$1" "$(command -v "$1")"
    else
        printf '[FAIL] %-12s not found\n' "$1"
        failures=$((failures + 1))
    fi
}

for command_name in python cmake ninja root root-config jupyter; do
    check_command "$command_name"
done

if command -v root-config >/dev/null 2>&1; then
    echo "[OK]   ROOT version $(root-config --version)"
fi

if [ -z "${CONDA_PREFIX:-}" ]; then
    echo "[FAIL] no active Micromamba environment"
    failures=$((failures + 1))
else
    for config_name in ROOTConfig.cmake TBBConfig.cmake; do
        config_path="$(
            find "$CONDA_PREFIX" -type f -name "$config_name" -print -quit \
                2>/dev/null
        )"
        if [ -n "$config_path" ]; then
            echo "[OK]   $config_name $config_path"
        else
            echo "[FAIL] $config_name not found below $CONDA_PREFIX"
            failures=$((failures + 1))
        fi
    done
fi

python - <<'PY' || failures=$((failures + 1))
import ROOT
import matplotlib
import numpy
import pandas
print(f"[OK]   PyROOT       {ROOT.__version__}")
PY

if [ "$failures" -ne 0 ]; then
    echo "Environment check failed with $failures problem(s)." >&2
    exit 1
fi

echo "Environment check passed."
