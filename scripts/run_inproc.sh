#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BIN="${BUILD_DIR}/benchmarks/e2e/inproc_runner"

SCENARIO="${1:-cross}"
N="${2:-200000}"
WARMUP="${3:-10000}"

"${BIN}" "${SCENARIO}" "${N}" "${WARMUP}"
