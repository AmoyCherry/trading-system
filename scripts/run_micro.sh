#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"
TS="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="${OUT_DIR}/micro_${TS}"

mkdir -p "${RUN_DIR}"

echo "== machine ==" | tee "${RUN_DIR}/machine.txt"
uname -a | tee -a "${RUN_DIR}/machine.txt" || true
command -v lscpu >/dev/null && lscpu | tee -a "${RUN_DIR}/machine.txt" || true
c++ --version | head -n 1 | tee -a "${RUN_DIR}/machine.txt" || true

# Disable pinning by set PIN=0
PIN="${PIN:-1}"
MICRO="${INPROC:-2}"
if [[ "${PIN}" == "1" ]]; then
  # `command -v` returns success iff `taskset` is on PATH (util-linux package).
  command -v taskset >/dev/null || {
    echo "taskset not found (install util-linux), or set PIN=0" >&2
    exit 3
  }
  MICRO_PREFIX=(taskset -c "${INPROC}")
else
  echo "No taskset -c"
  MICRO_PREFIX=()
fi

BIN="${BUILD_DIR}/benchmarks/micro/microbench"

echo "Running: ${BIN}"
"${MICRO_PREFIX[@]}" "${BIN}" \
  --benchmark_min_time=0.5 \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out="${RUN_DIR}/microbench.json" \
  --benchmark_out_format=json \
  | tee "${RUN_DIR}/microbench.txt"

echo "Saved results to ${RUN_DIR}"
