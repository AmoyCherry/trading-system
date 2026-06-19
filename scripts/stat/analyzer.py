"""
1. Guard
Read gateway.log to assert decode and seq errors are 0
Read all three .csv and assert the same count, min seq and max seq
Assert all intervals >= 0

Calc in- and cross-proc intervals within each repeat. Then calc these across repeats:
- mean
- MAD (the noise floor, socalled basic bg noise)
- CV=stddev/mean (normalized noise)

2. Process csv
Compute intervals:
- ex:
    - ex_intvl_send = after_send - before_send
- gw:
    - gw_intvl_decode = gw_before_send - gw_recv;
    - gw_intvl_send = gw_after_send - gw_before_send
- lobs:
    - lob_intvl_decode = lob_decode_done - lob_recv;
    - lob_intvl_apply = lob_apply_done - lob_decode_done
- ex2gw_trans = gw_recv − ex_after_send
- gw2lob_trans = lob_recv − gw_after_send
- w2w = lob_apply_done − ex_before_send
NOTE: Cross-process sub is only valid when they run on the same machine. On different machines this needs clock sync.

3. Process perf stat
Compute Mean/MAD/CV among repeats in each cell.
Read perf_stat.log. Then calculate:
- cycles/msg, get msgs from lobd.log
- branch-miss-rate
- cache-miss-rate
- Instr/Cycle can tell an op drops the cache-miss-rate and inc the IPC to reduce the CPU stalls

4. Cross-mode deltas

"""

import copy
from dataclasses import fields
from datetime import datetime
import pandas as pd

from scripts.stat.models import RepeatIntervals, LatencyCell, Stats
from scripts.stat.tools import get_val_from_log, med_mad, ROOT_DIR, load_counter_metrics, calc_interval_metrics, \
    stat_perf_repeats, med_quad_delta, decomp, write_source, stat_latency_repeats, estimate_gate_noise, write_md_table, \
    DECOMP_DOC, HEADLINE_DOC, NOISE_DOC

SCENARIOS = ["cross", "add", "cancel"]
MODES = ["null", "decode", "match"]
matrix_conf_path = ROOT_DIR / "scripts" / "e2e" / "matrix.conf"
REPEATS = get_val_from_log(matrix_conf_path, "REPEATS", "\n")

timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
summary_path = ROOT_DIR / "artifacts" / "summary" / timestamp
summary_path.mkdir(parents=True, exist_ok=True)

latency_summary_path = summary_path / "latency_summary.csv"
perf_summary_path = summary_path / "perf_summary.csv"

# View 1 - headline
headline = {
    'scenario': SCENARIOS,
    'throughput(M/s)': [0] * len(SCENARIOS),
    'p99_w2w(ms)': [0] * len(SCENARIOS),
    'p99_lob_apply(ns)': [0] * len(SCENARIOS),
    'lob_engine cyc/msg': [0] * len(SCENARIOS),
    'lob_codec cyc/msg': [0] * len(SCENARIOS),
    'floor cyc/msg': [0] * len(SCENARIOS)
}

# View 2 - w2w latency decomp
interval_means = [f.name for f in fields(LatencyCell) if f.name.endswith("mean_stat")]
def new_decomp_table():
    return {
        'metric': interval_means,
        'median (ns)': [0] * len(interval_means),
        'mad (ns)': [0] * len(interval_means),
        'robust cv': [0] * len(interval_means),
        '%w2w': [0] * len(interval_means),
    }
decomps = {s: new_decomp_table() for s in SCENARIOS}

# View 3
latency_gates = ['lob_intvl_apply_p99_stat', 'w2w_p99_stat']
perf_gates = ['throughput_stat']
gates = latency_gates + perf_gates
def new_noise_table():
    return {
        'metric': gates,
        'median': [0] * len(gates),
        'mad': [0] * len(gates),
        'robust cv': [0] * len(gates),
        'MDE': [0] * len(gates),
        'min-delta': [0] * len(gates)
    }
noises = {s: new_noise_table() for s in SCENARIOS}

with open(latency_summary_path, "w") as latency_sum, open(perf_summary_path, "w") as perf_sum:
    # todo! an UT to guarantee the stats schema with stat_latency_repeats and stat_perf_repeats
    latency_sum.write("scenario,interval,mean,std,median,mad,cv\n")
    perf_sum.write("scenario,mode,counter,mean,std,median,mad,cv\n")

    for s in SCENARIOS:
        si = SCENARIOS.index(s)
        scen_perf_cells = {}
        for m in MODES:
            latency_metrics_repeats = []
            counter_metrics_repeats = []
            for r in range(1, int(REPEATS)+1):
                counter_metrics = load_counter_metrics(s, m, r)
                counter_metrics_repeats.append(counter_metrics)

                if m == "match":
                    latency_metrics = calc_interval_metrics(s, m, r)
                    latency_metrics_repeats.append(latency_metrics)

            perf_cell = stat_perf_repeats(counter_metrics_repeats)
            for f in fields(perf_cell):
                # ['mean', 'std', 'median', mad, cv]
                stats = getattr(perf_cell, f.name)
                perf_sum.write(f"{s},{m},{f.name},{stats.mean},{stats.std},{stats.median},{stats.mad},{stats.robust_cv}\n")
            scen_perf_cells[m] = perf_cell

            if m == "match":
                latency_cell = stat_latency_repeats(latency_metrics_repeats)
                for f in fields(latency_cell):
                    stats = getattr(latency_cell, f.name)
                    latency_sum.write(f"{s},{f.name},{stats.mean},{stats.std},{stats.median},{stats.mad},{stats.robust_cv}\n")

                # View - scenarios x match headline
                headline['throughput(M/s)'][si] = med_mad(perf_cell.throughput_stat, 1e6)
                headline['p99_lob_apply(ns)'][si] = med_mad(latency_cell.lob_intvl_apply_p99_stat)
                headline['p99_w2w(ms)'][si] = med_mad(latency_cell.w2w_p99_stat, 1e6)
                headline['lob_engine cyc/msg'][si] = med_quad_delta(scen_perf_cells, 'cycle_per_msg_stat', 'match', 'decode')
                headline['lob_codec cyc/msg'][si] = med_quad_delta(scen_perf_cells, 'cycle_per_msg_stat', 'decode', 'null')
                headline['floor cyc/msg'][si] = med_mad(scen_perf_cells['null'].cycle_per_msg_stat)
                # View - latency decomp & noise floor
                decomp(decomps[s], latency_cell)
                estimate_gate_noise(noises[s], perf_gates, latency_cell, perf_cell)

view_path = summary_path / "view.md"
with open(view_path, "w") as view:
    write_md_table(view, "## Headline\n", headline)
    view.write(HEADLINE_DOC)

    view.write("## W2W Latency Decomposition\n")
    view.write(DECOMP_DOC)
    for s in SCENARIOS:
        write_md_table(view, f"### {s}\n", decomps[s])

    view.write("## Noise Floor\n")
    view.write(NOISE_DOC)
    for s in SCENARIOS:
        write_md_table(view, f"### {s}\n", noises[s])

source_path = summary_path / "source.txt"
write_source(source_path)

