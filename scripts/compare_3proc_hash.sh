#!/usr/bin/env bash
set -euo pipefail

SCENARIO="${1:-cross}"
N="${2:-200000}"

echo "Set scenario=${SCENARIO} n=${N}"


BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="${OUT_DIR:-artifacts/results}"

# Unique tag per run; used in artifacts dir AND socket paths so concurrent
# runs don't collide. $$ is the shell PID.
TS="$(date +%Y%m%d_%H%M%S)_$$"
RUN_DIR="${OUT_DIR}/compare_e2e_hash_${TS}"
mkdir -p "${RUN_DIR}"

# ---Launch replay_ref binaries ----------------------------------------------------------------
REPLAY_REF_BIN="${BUILD_DIR}/benchmarks/e2e/replay_ref"
"${REPLAY_REF_BIN}" "--scenario" "${SCENARIO}" "--n" "${N}" \
  > "${RUN_DIR}/replay_ref.log" 2>&1 &

# ---E2E socket paths ------------------------------------------------------------
# UDS pathname sockets, namespaced by TS, cleaned up in trap below.
BASE="/tmp/ts_${TS}"
EXCH="${BASE}_exch.sock"
GW="${BASE}_gw.sock"
LOB="${BASE}_lob.sock"

# ---E2E binaries ----------------------------------------------------------------
LOB_BIN="${BUILD_DIR}/src/lobd/lobd"
GW_BIN="${BUILD_DIR}/src/gateway/gateway"
EX_BIN="${BUILD_DIR}/src/exchange/exchange_sim"

# ---E2E timeouts ----------------------------------------------------------------
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

# --- launch ------------------------------------------------------------------
# Background each process (&), capture its PID ($!), redirect stdout+stderr
# to a per-process log inside RUN_DIR.

# 1. lobd  --- binds first; gateway needs its socket to exist before dialing.
"${LOB_BIN}" --local "${LOB}" \
  > "${RUN_DIR}/lobd.log" 2>&1 &
LOB_PID=$!
wait_for_ready "${RUN_DIR}/lobd.log" "lobd"

# 2. gateway  --- binds, dials lobd.
"${GW_BIN}" --local "${GW}" --to-lob "${LOB}" \
  > "${RUN_DIR}/gateway.log" 2>&1 &
GW_PID=$!
wait_for_ready "${RUN_DIR}/gateway.log" "gateway"

# 3. exchange_sim  --- foreground, hard-capped by `timeout`.
# `--foreground` forwards Ctrl-C to the child instead of swallowing it.
# `--signal=TERM` gives the child a chance to exit cleanly before SIGKILL.
set +e
timeout --foreground --signal=TERM "${EXCH_TIMEOUT}" \
  "${EX_BIN}" --local "${EXCH}" --to-gateway "${GW}" \
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


# Compare

# Define your files
REPLAY_REF_RESULT="${RUN_DIR}/replay_ref.log"
LOBD_RESULT="${RUN_DIR}/lobd.log"

echo "----------------replay_ref result----------------"
cat "$REPLAY_REF_RESULT"

echo "----------------e2e lobd result----------------"
cat "$LOBD_RESULT"

# Ensure both files exist before proceeding
if [[ ! -f "$REPLAY_REF_RESULT" ]] || [[ ! -f "$LOBD_RESULT" ]]; then
    echo "Error: One or both files do not exist."
    exit 1
fi

# Extract the hash values.
# We use grep with Perl-compatible regex (-P) and output only the match (-o).
# \K means "ignore everything matched up to this point", leaving just the hash.
hash1=$(grep -oP 'state_hash=\K[0-9]+' "$REPLAY_REF_RESULT")
hash2=$(grep -oP 'state_hash=\K[0-9]+' "$LOBD_RESULT")

# Check if we actually found a hash in both files
if [[ -z "$hash1" ]] || [[ -z "$hash2" ]]; then
    echo "Error: Could not find 'state_hash=' in one or both files."
    exit 1
fi

echo "---------------------------------"
echo "replay_ref hash: $hash1"
echo "e2e lobd hash: $hash2"
echo "---------------------------------"

# Compare the strings
if [[ "$hash1" == "$hash2" ]]; then
    echo "✅ Success: The state hashes match."
else
    echo "❌ Mismatch: The state hashes are different."
fi

