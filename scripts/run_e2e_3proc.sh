#!/usr/bin/env bash
# Launch lobd -> gateway -> exchange_sim and collect per-process summaries.
# Order: lobd binds first, gateway second (it dials lobd), exchange_sim last
# (it dials gateway and produces the order stream + EndOfReplay).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"
TS="$(date +%Y%m%d_%H%M%S)_$$"
RUN_DIR="${OUT_DIR}/e2e3_${TS}"
mkdir -p "${RUN_DIR}"

BASE="/tmp/ts_${TS}"
EXCH="${BASE}_exch.sock"
GW="${BASE}_gw.sock"
LOB="${BASE}_lob.sock"

LOB_BIN="${BUILD_DIR}/src/lobd/lobd"
GW_BIN="${BUILD_DIR}/src/gateway/gateway"
EX_BIN="${BUILD_DIR}/src/exchange/exchange_sim"

SCENARIO="${1:-cross}"
N="${2:-2000000}"
READY_TIMEOUT="${READY_TIMEOUT:-5}"
EXCH_TIMEOUT="${EXCH_TIMEOUT:-60}"
DRAIN_TIMEOUT="${DRAIN_TIMEOUT:-10}"

for bin in "${LOB_BIN}" "${GW_BIN}" "${EX_BIN}"; do
  if [[ ! -x "${bin}" ]]; then
    echo "missing binary: ${bin} - run scripts/build.sh first" >&2
    exit 2
  fi
done

LOB_PID=""
GW_PID=""

cleanup() {
  set +e
  [[ -n "${GW_PID}" ]] && kill -KILL "${GW_PID}" 2>/dev/null
  [[ -n "${LOB_PID}" ]] && kill -KILL "${LOB_PID}" 2>/dev/null
  rm -f "${EXCH}" "${GW}" "${LOB}"
}
trap cleanup EXIT

wait_for_ready() {
  local log_file="$1" name="$2"
  local steps=$(( READY_TIMEOUT * 20 ))
  for _ in $(seq 1 ${steps}); do
    if grep -q '^READY ' "$log_file" 2>/dev/null; then
      return 0
    fi
    sleep 0.05
  done
  echo "${name}: timed out waiting for READY (see ${log_file})" >&2
  return 1
}

# Wait up to deadline_s for pid to exit. Returns 0 if exited, 1 on timeout.
wait_pid_exit() {
  local pid="$1" deadline_s="$2"
  local steps=$(( deadline_s * 20 ))
  for _ in $(seq 1 ${steps}); do
    kill -0 "${pid}" 2>/dev/null || return 0
    sleep 0.05
  done
  return 1
}

# SIGTERM, brief wait, then SIGKILL if still alive.
stop_pid() {
  local pid="$1"
  [[ -z "${pid}" ]] && return 0
  kill -0 "${pid}" 2>/dev/null || return 0
  kill -TERM "${pid}" 2>/dev/null || true
  wait_pid_exit "${pid}" 2 && return 0
  kill -KILL "${pid}" 2>/dev/null || true
}

# 1. lobd
"${LOB_BIN}" --local "${LOB}" > "${RUN_DIR}/lobd.log" 2>&1 &
LOB_PID=$!
wait_for_ready "${RUN_DIR}/lobd.log" "lobd"

# 2. gateway
"${GW_BIN}" --local "${GW}" --to-lob "${LOB}" > "${RUN_DIR}/gateway.log" 2>&1 &
GW_PID=$!
wait_for_ready "${RUN_DIR}/gateway.log" "gateway"

# 3. exchange_sim (foreground, with hard timeout to prevent silent hangs)
echo "running exchange_sim: scenario=${SCENARIO} n=${N}"
set +e
timeout --foreground --signal=TERM "${EXCH_TIMEOUT}" \
  "${EX_BIN}" --local "${EXCH}" --to-gateway "${GW}" --scenario "${SCENARIO}" --n "${N}" \
  | tee "${RUN_DIR}/exchange.out"
EX_STATUS=${PIPESTATUS[0]}
set -e

case "${EX_STATUS}" in
  0)   ;;
  124) echo "exchange_sim timed out after ${EXCH_TIMEOUT}s" >&2 ;;
  *)   echo "exchange_sim exited with status ${EX_STATUS}" >&2 ;;
esac

# Drain: gateway forwards EndOfReplay to lobd, both should exit.
wait_pid_exit "${GW_PID}"  "${DRAIN_TIMEOUT}" || echo "gateway did not exit within ${DRAIN_TIMEOUT}s; sending TERM" >&2
wait_pid_exit "${LOB_PID}" "${DRAIN_TIMEOUT}" || echo "lobd did not exit within ${DRAIN_TIMEOUT}s; sending TERM" >&2

stop_pid "${GW_PID}"
stop_pid "${LOB_PID}"

wait "${GW_PID}"  2>/dev/null || true
wait "${LOB_PID}" 2>/dev/null || true

printf '\n== exchange ==\n'
grep '^RESULT' "${RUN_DIR}/exchange.out" || echo "(no RESULT line)"
printf '\n== gateway ==\n'
grep '^RESULT' "${RUN_DIR}/gateway.log" || echo "(no RESULT line)"
printf '\n== lobd ==\n'
grep -E '^(STATS|RESULT)' "${RUN_DIR}/lobd.log" || echo "(no STATS/RESULT line)"

echo
echo "RUN_DIR=${RUN_DIR}"
