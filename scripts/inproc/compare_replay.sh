#!/usr/bin/env bash
# Determinism check: run replay_ref twice with the same scenario+n and verify
# the outputs are byte-identical. A pass means the scenario generator + engine
# produce a stable order stream and a stable book state.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# Configure inputs here
SCENARIO="${SCENARIO:-cross}"
N="${N:-2000000}"
BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"

REF_BIN="${BUILD_DIR}/benchmarks/e2e/replay_ref"
if [[ ! -x "${REF_BIN}" ]]; then
  echo "replay_ref not found at ${REF_BIN} - run scripts/build.sh first" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)_$$"
RUN_DIR="${OUT_DIR}/replay_compare_${TS}"
mkdir -p "${RUN_DIR}"

A="${RUN_DIR}/run_a.out"
B="${RUN_DIR}/run_b.out"

echo "scenario=${SCENARIO} n=${N}"
echo "run A -> ${A}"
"${REF_BIN}" --scenario "${SCENARIO}" --n "${N}" > "${A}"
echo "run B -> ${B}"
"${REF_BIN}" --scenario "${SCENARIO}" --n "${N}" > "${B}"

if diff -q "${A}" "${B}" >/dev/null; then
  echo "MATCH"
  echo "RUN_DIR=${RUN_DIR}"
  exit 0
else
  echo "NOT MATCH!!!"
fi

echo "MISMATCH" >&2
diff -u "${A}" "${B}" || true
echo "RUN_DIR=${RUN_DIR}" >&2
exit 1
