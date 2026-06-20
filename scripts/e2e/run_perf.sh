#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TS="$(date +%Y%m%d_%H%M%S)_$$"

"$SCRIPT_DIR"/run_3proc.sh --scenario cross --mode match --stride 0 --n 4600000 --cpu-core \
                           --ts "$TS" --perf