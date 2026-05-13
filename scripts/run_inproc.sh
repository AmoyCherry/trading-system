#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BIN="${BUILD_DIR}/benchmarks/e2e/inproc_runner"

SCENARIO="${1:-cross}"
N="${2:-20000000}"
WARMUP="${3:-100000}"

# Disable pinning by set PIN=0
PIN="${PIN:-1}"
INPROC="${INPROC:-2}"
if [[ "${PIN}" == "1" ]]; then
  # `command -v` returns success iff `taskset` is on PATH (util-linux package).
  command -v taskset >/dev/null || {
    echo "taskset not found (install util-linux), or set PIN=0" >&2
    exit 3
  }
  INPROC_PREFIX=(taskset -c "${INPROC}")
else
  INPROC_PREFIX=()
fi

"${INPROC_PREFIX[@]}" "${BIN}" "${SCENARIO}" "${N}" "${WARMUP}"
