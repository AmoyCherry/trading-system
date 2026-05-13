#!/usr/bin/env bash
# =============================================================================
# 3-process E2E runner with CPU pinning.
#
# Pipeline:    exchange_sim --> gateway --> lobd
#
# Launch order:    lobd  (binds, waits)
#               -> gateway (binds, dials lobd)
#               -> exchange_sim (dials gateway, produces stream + EndOfReplay)
#
# Order matters: each process is started only AFTER its downstream peer has
# printed a "READY ..." line. This prevents a sender from blasting packets
# into a not-yet-bound socket and silently dropping them.
#
# Why pin?  Three processes sharing one core means the kernel context-switches
# between them. That adds tens of microseconds of jitter to p99 latency and
# corrupts perf-counter attribution. Each process gets its own core via
# `taskset -c <N>`. For *clean* numbers you also need governor=performance,
# Turbo off, and ideally isolcpus= for the pinned cores
# (see docs/experiments/000_template.md, "Environment").
# =============================================================================

set -euo pipefail
# -e: exit on any error
# -u: unbound variables are errors (catches typos)
# -o pipefail: a pipe's exit status is the rightmost non-zero, not just the last

# --- paths -------------------------------------------------------------------
# Resolve the script's own dir, then cd to the repo root. This lets the
# script run from anywhere ("./scripts/run_e2e_3proc.sh", "bash scripts/...", etc.).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# Env-overridable knobs use the ${VAR:-default} idiom: VAR if set, else default.
BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"

# Unique tag per run; used in artifacts dir AND socket paths so concurrent
# runs don't collide. $$ is the shell PID.
TS="$(date +%Y%m%d_%H%M%S)_$$"
RUN_DIR="${OUT_DIR}/e2e3_${TS}"
mkdir -p "${RUN_DIR}"

# --- socket paths ------------------------------------------------------------
# UDS pathname sockets, namespaced by TS, cleaned up in trap below.
BASE="/tmp/ts_${TS}"
EXCH="${BASE}_exch.sock"
GW="${BASE}_gw.sock"
LOB="${BASE}_lob.sock"

# --- binaries ----------------------------------------------------------------
LOB_BIN="${BUILD_DIR}/src/lobd/lobd"
GW_BIN="${BUILD_DIR}/src/gateway/gateway"
EX_BIN="${BUILD_DIR}/src/exchange/exchange_sim"

# --- positional args ---------------------------------------------------------
SCENARIO="${1:-cross}"          # cross | add_only | cancel_heavy
N="${2:-2000000}"               # number of scenario units

# --- CPU pinning -------------------------------------------------------------
# Default cores: skip 0 (kernel housekeeping commonly lands there) and pick
# three non-adjacent cores to reduce the chance of hitting SMT siblings on
# CPUs where siblings are numbered N and N+1 (some Intel) or N and N+core_count
# (some AMD/recent Intel). Override per run, e.g.:
#     LOB_CORE=4 GW_CORE=6 EX_CORE=8 ./scripts/run_e2e_3proc.sh
# Set PIN=0 to disable; useful as a "no-pin baseline" experiment.
PIN="${PIN:-1}"
LOB_CORE="${LOB_CORE:-2}"
GW_CORE="${GW_CORE:-3}"
EX_CORE="${EX_CORE:-4}"

# Resolve the taskset prefix once, as a bash array. When PIN=0 the array is
# empty and expands to nothing in the launch commands below.
# Bash arrays: declared with (), expanded with "${name[@]}" to preserve word splits.
if [[ "${PIN}" == "1" ]]; then
  # `command -v` returns success iff `taskset` is on PATH (util-linux package).
  command -v taskset >/dev/null || {
    echo "taskset not found (install util-linux), or set PIN=0" >&2
    exit 3
  }
  LOB_PREFIX=(taskset -c "${LOB_CORE}")
  GW_PREFIX=(taskset -c "${GW_CORE}")
  EX_PREFIX=(taskset -c "${EX_CORE}")
  echo "pinning: lobd=core${LOB_CORE} gateway=core${GW_CORE} exchange=core${EX_CORE}"
else
  LOB_PREFIX=()
  GW_PREFIX=()
  EX_PREFIX=()
  echo "pinning: OFF"
fi

# --- timeouts ----------------------------------------------------------------
READY_TIMEOUT="${READY_TIMEOUT:-5}"     # seconds to wait for a READY line
EXCH_TIMEOUT="${EXCH_TIMEOUT:-60}"      # hard cap for exchange_sim
DRAIN_TIMEOUT="${DRAIN_TIMEOUT:-10}"    # how long to wait for downstream drain

# --- preflight ---------------------------------------------------------------
# Fail fast with a useful message if anything is missing.
for bin in "${LOB_BIN}" "${GW_BIN}" "${EX_BIN}"; do
  if [[ ! -x "${bin}" ]]; then
    echo "missing binary: ${bin} - run scripts/build.sh first" >&2
    exit 2
  fi
done

# --- cleanup -----------------------------------------------------------------
# `trap ... EXIT` runs the cleanup on any exit path (success, error, Ctrl-C).
# This guarantees we don't leave orphan procs or stale sockets even on Ctrl-C.
LOB_PID=""
GW_PID=""

cleanup() {
  set +e   # don't let cleanup errors abort cleanup itself
  [[ -n "${GW_PID}"  ]] && kill -KILL "${GW_PID}"  2>/dev/null
  [[ -n "${LOB_PID}" ]] && kill -KILL "${LOB_PID}" 2>/dev/null
  rm -f "${EXCH}" "${GW}" "${LOB}"
}
trap cleanup EXIT

# --- helpers -----------------------------------------------------------------
# Poll the log file for the first "READY " line. Polling (not inotify) keeps
# this portable; resolution is ~50ms which is plenty for a startup handshake.
wait_for_ready() {
  local log_file="$1" name="$2"
  local steps=$(( READY_TIMEOUT * 20 ))   # 20 ticks/sec * READY_TIMEOUT sec
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
# `kill -0 PID` doesn't send a signal; it just asks "does PID exist and am I
# allowed to signal it?" -- the cheapest existence check.
wait_pid_exit() {
  local pid="$1" deadline_s="$2"
  local steps=$(( deadline_s * 20 ))
  for _ in $(seq 1 ${steps}); do
    kill -0 "${pid}" 2>/dev/null || return 0
    sleep 0.05
  done
  return 1
}

# Graceful stop: SIGTERM, brief wait, then SIGKILL if still alive.
# A well-behaved process traps SIGTERM and flushes logs; SIGKILL is the
# nuclear fallback for hung procs.
stop_pid() {
  local pid="$1"
  [[ -z "${pid}" ]] && return 0
  kill -0 "${pid}" 2>/dev/null || return 0
  kill -TERM "${pid}" 2>/dev/null || true
  wait_pid_exit "${pid}" 2 && return 0
  kill -KILL "${pid}" 2>/dev/null || true
}

# Print the kernel's view of a PID's CPU affinity. Sanity check that taskset
# actually narrowed the mask (useful when debugging cgroup overrides).
log_affinity() {
  local pid="$1" name="$2"
  [[ "${PIN}" == "1" ]] || return 0
  taskset -p "${pid}" 2>/dev/null | sed "s/^/  affinity[${name}]: /"
}

# --- launch ------------------------------------------------------------------
# Background each process (&), capture its PID ($!), redirect stdout+stderr
# to a per-process log inside RUN_DIR.

# 1. lobd  --- binds first; gateway needs its socket to exist before dialing.
"${LOB_PREFIX[@]}" "${LOB_BIN}" --local "${LOB}" \
  > "${RUN_DIR}/lobd.log" 2>&1 &
LOB_PID=$!
wait_for_ready "${RUN_DIR}/lobd.log" "lobd"
log_affinity "${LOB_PID}" "lobd"

# 2. gateway  --- binds, dials lobd.
"${GW_PREFIX[@]}" "${GW_BIN}" --local "${GW}" --to-lob "${LOB}" \
  > "${RUN_DIR}/gateway.log" 2>&1 &
GW_PID=$!
wait_for_ready "${RUN_DIR}/gateway.log" "gateway"
log_affinity "${GW_PID}" "gateway"

# 3. exchange_sim  --- foreground, hard-capped by `timeout`.
# `--foreground` forwards Ctrl-C to the child instead of swallowing it.
# `--signal=TERM` gives the child a chance to exit cleanly before SIGKILL.
echo "running exchange_sim: scenario=${SCENARIO} n=${N}"
set +e
timeout --foreground --signal=TERM "${EXCH_TIMEOUT}" \
  "${EX_PREFIX[@]}" "${EX_BIN}" --local "${EXCH}" --to-gateway "${GW}" \
    --scenario "${SCENARIO}" --n "${N}" \
  | tee "${RUN_DIR}/exchange.out"
# `$?` would be `tee`'s exit code; we want exchange_sim's, which is in
# the first slot of PIPESTATUS (an array of every pipe stage's exit code).
EX_STATUS=${PIPESTATUS[0]}
set -e

case "${EX_STATUS}" in
  0)   ;;  # OK
  124) echo "exchange_sim timed out after ${EXCH_TIMEOUT}s" >&2 ;;
  *)   echo "exchange_sim exited with status ${EX_STATUS}" >&2 ;;
esac

# --- drain -------------------------------------------------------------------
# Exchange has sent EndOfReplay. Gateway forwards it; lobd prints RESULT and
# exits. We wait DRAIN_TIMEOUT for each; anything still alive after that is
# stuck and gets TERM/KILL.
wait_pid_exit "${GW_PID}"  "${DRAIN_TIMEOUT}" \
  || echo "gateway did not exit within ${DRAIN_TIMEOUT}s; sending TERM" >&2
wait_pid_exit "${LOB_PID}" "${DRAIN_TIMEOUT}" \
  || echo "lobd did not exit within ${DRAIN_TIMEOUT}s; sending TERM" >&2

stop_pid "${GW_PID}"
stop_pid "${LOB_PID}"

# `wait` reaps zombies so the EXIT trap doesn't print stray "killed" messages.
wait "${GW_PID}"  2>/dev/null || true
wait "${LOB_PID}" 2>/dev/null || true

# --- report ------------------------------------------------------------------
printf '\n== exchange ==\n'
grep '^RESULT' "${RUN_DIR}/exchange.out" || echo "(no RESULT line)"
printf '\n== gateway ==\n'
grep '^RESULT' "${RUN_DIR}/gateway.log" || echo "(no RESULT line)"
printf '\n== lobd ==\n'
grep -E '^(STATS|RESULT)' "${RUN_DIR}/lobd.log" || echo "(no STATS/RESULT line)"

echo
echo "RUN_DIR=${RUN_DIR}"
