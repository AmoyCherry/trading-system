#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"
TS="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="${OUT_DIR}/perf_${TS}"

mkdir -p "${RUN_DIR}"

BIN="${BUILD_DIR}/benchmarks/micro/microbench"
FILTER="${1:-BM_MatchSweep}"

# Pin to one CPU if you want (edit CPU=2 etc.)
CPU="${CPU:-}"

CMD=( "${BIN}" --benchmark_filter="${FILTER}" --benchmark_min_time=1.0s )

if [[ -n "${CPU}" ]]; then
  CMD=( taskset -c "${CPU}" "${CMD[@]}" )
fi

echo "Command: ${CMD[*]}" | tee "${RUN_DIR}/cmd.txt"

perf stat -r 5 \
  -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,context-switches,cpu-migrations \
  -- "${CMD[@]}" \
  &> "${RUN_DIR}/perf_stat.txt"

cat "${RUN_DIR}/perf_stat.txt"

echo "Saved perf to ${RUN_DIR}/perf_stat.txt"
