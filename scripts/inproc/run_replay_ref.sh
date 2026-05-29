#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BIN="${BUILD_DIR}/benchmarks/e2e/replay_ref"

SCENARIO="${1:-cross}"
N="${2:-200000}"

"${BIN}" "--scenario" "${SCENARIO}" "--n" "${N}"
