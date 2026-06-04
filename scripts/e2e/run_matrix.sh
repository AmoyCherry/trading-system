#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -f "${SCRIPT_DIR}/matrix.conf" ]; then
    source "${SCRIPT_DIR}/matrix.conf"
else
    echo "Error: ${SCRIPT_DIR}/matrix.conf not found!"
    exit 1
fi

TS="$(date +%Y%m%d_%H%M%S)_$$"

PERF_FLAG=()
if [[ "${PERF}" -eq 1 ]]; then
  PERF_FLAG="--perf"
fi

# Restore env-hygiene: scaling governor; no turbo;
cleanup() {
    echo ""
    echo "=== Cleaning Up Environment ==="

    # Restoring Governor
    echo "Switching CPU governor back to 'powersave'..."
    for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        echo "powersave" | sudo tee "$i" > /dev/null
    done

    # Restore EPP (Intel default balance_performance for daily use)
    if ls /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference >/dev/null 2>&1; then
        echo "Restoring Intel EPP to 'balance_performance'..."
        for i in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do
            echo "balance_performance" | sudo tee "$i" > /dev/null
        done
    fi

    # Restoring Turbo Boost (Enabling it back for normal use)
    echo "Re-enabling Intel Turbo Boost..."
    echo "0" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null

    echo "✅ Environment completely restored to default hygiene."
}
# --- Register the Trap ---
# Catches normal exit, Ctrl+C, or kill signals to guarantee cleanup runs
trap cleanup EXIT SIGINT SIGTERM

set_env_hygiene() {
    echo "=== Initializing Hybrid Environment Hygiene (i7-1360P) ==="

    # Assert Perf Paranoid level
    if [ -f /proc/sys/kernel/perf_event_paranoid ]; then
        local paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)
        if [ "$paranoid" -gt 1 ]; then
            echo "❌ ERROR: perf_event_paranoid is set to $paranoid, must be 1 or lower for perf profiling."
            echo "   Fix with: sudo sysctl -w kernel.perf_event_paranoid=1"
            return 1
        fi
        echo "✅ Assertion Passed: perf_event_paranoid is $paranoid"
    fi

    # Set CPU Scaling Governor to Performance
    echo "Locking CPU scaling governors to 'performance'..."
    for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        echo "performance" | sudo tee "$gov" > /dev/null
    done

    # Set Energy Performance Preference (EPP) to Performance
    if ls /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference >/dev/null 2>&1; then
        echo "Locking Intel EPP to 'performance'..."
        for epp in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do
            echo "performance" | sudo tee "$epp" > /dev/null
        done
    fi

    echo "Disabling Intel Turbo Boost..."
    echo "1" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null

    echo "=== Environment Hygiene Successfully Configured ==="
    return 0
}
set_env_hygiene || exit 1


for scenario in "${SCENARIOS[@]}"; do
    for mode in "${MODES[@]}"; do
      for ((run=1; run<=REPEATS; run++)); do

        echo "------------------ matrix ------------------------------"
        echo "Running: Scenario=$scenario | Mode=$mode | Repeats=$REPEATS | N=$N | Stride=$STRIDE"
        echo "------------------ matrix ------------------------------"

        "$SCRIPT_DIR"/run_3proc.sh --scenario "$scenario" --mode "$mode" --stride "$STRIDE" --n "$N" --repeat "$run" --cpu-core \
                                   --ts "$TS" "${PERF_FLAG[@]}"

        echo -e "\n"
      done # repeat
    done # mode
done # scenario