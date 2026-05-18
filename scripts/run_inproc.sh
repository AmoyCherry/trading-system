#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BIN="${BUILD_DIR}/benchmarks/e2e/inproc_runner"

# --- argument parsing -------------------------------------------------------
# Phase 1: walk $@, consume known flags, push everything else into POSITIONAL.
# Phase 2: reset $@ from POSITIONAL, then assign $1, $2, ... as before.

USE_IN_BINARY_AFFINITY=0
CPU_CORE=""            # empty = "use env-var default"
POSITIONAL=()
INPROC="${INPROC:-2}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    # Form A: `--cpu-core 2`  (flag + separate value)
    --cpu-core)
      USE_IN_BINARY_AFFINITY=1
      # Consume the next token as the value ONLY if it looks numeric.
      # This is what prevents `--cpu-core --n` from eating the next flag.
      if [[ $# -ge 2 && "$2" =~ ^[0-9]+$ ]]; then
        CPU_CORE="$2"
        shift 2
      else
        shift          # bare toggle, no value
      fi
      ;;

    # Form B: `--cpu-core=2`  (one token, '=' separator)
    --cpu-core=*)
      USE_IN_BINARY_AFFINITY=1
      CPU_CORE="${1#--cpu-core=}"   # strip the "--cpu-core=" prefix
      shift
      ;;

    # End-of-options marker: everything after is positional, even if it
    # starts with `--`. Standard POSIX convention.
    --)
      shift
      POSITIONAL+=("$@")
      break
      ;;

    # Unknown flag: fail loudly. Better than silently passing through.
    --*)
      echo "unknown flag: $1" >&2
      exit 2
      ;;

    # Plain positional
    *)
      POSITIONAL+=("$1")
      shift
      ;;
  esac
done

# Replace $@ with just the positionals. Now $1, $2, ... are clean.
set -- "${POSITIONAL[@]}"

# Phase 2: assign positionals exactly as you already do.
SCENARIO="${1:-cross}"
N="${2:-20000000}"
WARMUP="${3:-100000}"

# Resolve the actual core: explicit flag value wins; else env default.
if [[ "${USE_IN_BINARY_AFFINITY}" -eq 1 ]]; then
  INPROC_CORE=(--cpu-core "${CPU_CORE:-${INPROC}}")
  INPROC_PREFIX=()
  echo "use in-binary cpu pinning: sche_setaffinity ${CPU_CORE}"
else
    # `command -v` returns success iff `taskset` is on PATH (util-linux package).
    command -v taskset >/dev/null || {
      echo "taskset not found" >&2
      exit 3
    }
    INPROC_CORE=()
    INPROC_PREFIX=(taskset -c "${INPROC}")
    echo "taskset -c pinning: ${CPU_CORE}"
fi

"${INPROC_PREFIX[@]}" "${BIN}" --scenario "${SCENARIO}" --n "${N}" --warmup "${WARMUP}" "${INPROC_CORE[@]}"
