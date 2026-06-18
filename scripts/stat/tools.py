import math

import pandas as pd
import numpy as np
import json
from dataclasses import dataclass, field, fields
from pathlib import Path

from scripts.stat.models import RepeatIntervals, RepeatIntervalMetrics, PerfCell, LatencyCell, CounterMetrics

ROOT_DIR = Path(__file__).resolve().parent.parent.parent
RESULTS_DIR = ROOT_DIR / "artifacts" / "results"

def get_latest_result_dir(prefix: str) -> Path:
    """
    Scans the artifact directory and returns the Path to the folder
    with the latest timestamp in its name.
    """
    # 1. Glob all directories matching the pattern 'matrix_*'
    artifact_folders = [f for f in RESULTS_DIR.glob(f"{prefix}_*") if f.is_dir()]

    if not artifact_folders:
        raise FileNotFoundError(f"No artifact folders matching '{prefix}_*' found in {RESULTS_DIR}")

    # 2. Use max() with the folder name string as the key.
    # Because '20260602' naturally sorts before '20260606', max() returns the newest one.
    latest_folder = max(artifact_folders, key=lambda folder: folder.name)

    return latest_folder

latest_latency_dir = get_latest_result_dir("latency")
latest_perf_dir = get_latest_result_dir("perf")

def get_csv_path(category, scenario, mode, repeat, csv) -> Path:
    return category / scenario / mode / f"repeat_{repeat}" / f"{csv}.csv"

def get_log_path(category, scenario, mode, repeat, log) -> Path:
    return category / scenario / mode / f"repeat_{repeat}" / f"{log}"

# ------------- latency -----------------------------------------

def load_repeat_intervals(scenario, mode, repeat) -> RepeatIntervals:
    intervals = RepeatIntervals()

    ex_csv_path = get_csv_path(latest_latency_dir, scenario, mode, repeat, "exts")
    ex_csv = pd.read_csv(ex_csv_path)
    ex_seq = ex_csv["seq"].to_list()
    intervals.ex_intvl_send = (ex_csv["ex_after_send"] - ex_csv["ex_before_send"]).to_list()

    gw_csv_path = get_csv_path(latest_latency_dir, scenario, mode, repeat, "gwts")
    gw_csv = pd.read_csv(gw_csv_path)
    gw_seq = gw_csv["seq"].to_list()
    intervals.gw_intvl_decode = (gw_csv["gw_before_send"] - gw_csv["gw_recv"]).to_list()
    intervals.gw_intvl_send = (gw_csv["gw_after_send"] - gw_csv["gw_before_send"]).to_list()

    lob_csv_path = get_csv_path(latest_latency_dir, scenario, mode, repeat, "lobts")
    lob_csv = pd.read_csv(lob_csv_path)
    lob_seq = lob_csv["seq"].to_list()
    intervals.lob_intvl_decode = (lob_csv["lob_decode_done"] - lob_csv["lob_recv"]).to_list()
    intervals.lob_intvl_apply = (lob_csv["lob_apply_done"] - lob_csv["lob_decode_done"]).to_list()

    intervals.ex2gw_trans = (gw_csv["gw_recv"] - ex_csv["ex_after_send"]).to_list()
    intervals.gw2lob_trans = (lob_csv["lob_recv"] - gw_csv["gw_after_send"]).to_list()
    intervals.w2w = (lob_csv["lob_apply_done"] - ex_csv["ex_before_send"]).to_list()

    # todo! remove these manual align
    gw_seq.pop()
    intervals.gw_intvl_decode.pop()
    intervals.gw_intvl_send.pop()
    intervals.ex2gw_trans.pop()
    intervals.gw2lob_trans.pop()

    assert ex_seq == gw_seq == lob_seq, f"seq match failure\n s:{scenario} m:{mode}, r:{repeat}\n ex_seq len: {len(ex_seq)}, gw_seq len: {len(gw_seq)},  lob_seq len, {len(lob_seq)}"

    return intervals

def get_val_from_log(path: str, key: str, sep: str):
    with open(path, 'r') as f:
        item = next((token for token in f.read().split(sep) if token.startswith(key)), None)
        if item:
            val = item.split("=")[1]
        else:
            raise ValueError(f"No {key} info found in {path}")

    return val


def calc_interval_metrics(scenario, mode, repeat) -> RepeatIntervalMetrics:
    intervals = load_repeat_intervals(scenario, mode, repeat)

    kwargs = {}

    for f in fields(intervals):
        data = getattr(intervals, f.name)
        data.sort()

        p50, p99, p_max = np.percentile(data, [50, 99, 100])
        mean = np.mean(data)

        kwargs[f"{f.name}_p50"] = p50
        kwargs[f"{f.name}_p99"] = p99
        kwargs[f"{f.name}_max"] = p_max
        kwargs[f"{f.name}_mean"] = mean

    return RepeatIntervalMetrics(**kwargs)

# Median Abs Deviation
def mad(arr):
    return np.median(np.abs(arr - np.median(arr)))

def cv(arr):
    return np.std(arr) / np.mean(arr)

def stat_latency_repeats(latency_metrics_repeats: list[RepeatIntervalMetrics]) -> LatencyCell:
    df = pd.DataFrame(latency_metrics_repeats)
    summary_stats = df.agg(['mean', 'std', 'median', mad, cv])

    # summary_stats.to_dict('list') produces:
    # {'ex_intvl_send_p50': [10.5, 1.2, 0.11], 'gw_intvl_decode_p50': [...], ...}
    cell_kwargs = {
        f"{col}_stat": values
        for col, values in summary_stats.to_dict('list').items()
    }

    return LatencyCell(**cell_kwargs)


# ------------- perf counters -----------------------------------------

def get_value_from_config(config_path: str, key: str) -> int:
    with open(config_path, 'r') as f:
        config = json.load(f)

    return config[key]

def load_counter_metrics(scenario, mode, repeat) -> CounterMetrics:
    config_path = get_log_path(latest_perf_dir, scenario, mode, repeat, "config.json")
    msgs = get_value_from_config(str(config_path), 'n')

    perf_path = get_log_path(latest_perf_dir, scenario, mode, repeat, "perf_stat.log")
    raw_data = {}
    with open(perf_path, 'r') as f:
        for line in f:
            parts = line.split()

            # Fast-fail filters: Need exact 2 columns, must be a P-core event
            if len(parts) == 2 and parts[1].startswith('cpu_core/'):
                val_str = parts[0].replace(',', '') # Strip the commas from the numbers

                if val_str.isdigit():
                    # Extracts 'branch-misses' from 'cpu_core/branch-misses/'
                    # and converts to 'branch_misses' to match your dataclass fields
                    event_name = parts[1].split('/')[1].replace('-', '_')
                    raw_data[event_name] = int(val_str)

    ex_path = get_log_path(latest_perf_dir, scenario, mode, repeat, "exchange.out")
    throughput = get_val_from_log(str(ex_path), "send_throughput_msgs_per_s", " ")

    def safe_div(num: float, den: float) -> float:
        return float(num) / float(den) if den else 0.0

    # No default val to destroy results, fail loud!
    return CounterMetrics(
        ipc=safe_div(raw_data['instructions'], raw_data['cycles']),
        cycle_per_msg=safe_div(raw_data['cycles'], msgs),
        cache_miss_rate=safe_div(raw_data['cache_misses'], raw_data['cache_references']),
        branch_miss_rate=safe_div(raw_data['branch_misses'], raw_data['branches']),
        throughput=int(throughput)
    )

def stat_perf_repeats(counters_repeats: list[CounterMetrics]) -> PerfCell:
    df = pd.DataFrame(counters_repeats)
    summary_stats = df.agg(['mean', 'std', 'median', mad, cv])

    cell_kwargs = {
        f"{col}_stat": values
        for col, values in summary_stats.to_dict('list').items()
    }

    return PerfCell(**cell_kwargs)


# ------------- summary -----------------------------------------
def med_mad(stats: list[float], div = 1) -> str:
    # ['mean', 'std', 'median', mad, cv]
    return f"{(stats[2] / div):.2f} +- {(stats[3]) / div:.2f}"

def med_quad_delta(scen_perf_cells: dict[str, PerfCell], metric_name: str, mode_l: str, mode_s: str) -> str:
    metrics_l = getattr(scen_perf_cells[mode_l], metric_name)
    metrics_s = getattr(scen_perf_cells[mode_s], metric_name)

    med_l = metrics_l[2]
    med_s = metrics_s[2]

    std_l = metrics_l[1]
    std_s = metrics_s[1]

    return f"{(med_l - med_s):.3f} +- {math.hypot(std_l, std_s):.3f}"

def decomp(cols: dict[str, list], latency_cell: LatencyCell):
    # ['mean', 'std', 'median', mad, cv]
    w2w_mean_med = latency_cell.w2w_mean_stat[2]
    for idx, metric_name in enumerate(cols['metric']):
        stats = getattr(latency_cell, metric_name)
        cols['median (ns)'][idx] = f"{stats[2]}"
        cols['mad (ns)'][idx] = round(stats[3], 3)
        cols['cv'][idx] = round(stats[4], 3)
        cols['%w2w'][idx] = f"{(stats[2] / w2w_mean_med):.2%}"

K = 3
def estimate_gate_noise(cols: dict[str, list], perf_metrics: list[str], latency_cell: LatencyCell, perf_cell: PerfCell):
    for idx, metric_name in enumerate(cols['metric']):
        if metric_name in perf_metrics:
            stats = getattr(perf_cell, metric_name)
        else:
            stats = getattr(latency_cell, metric_name)

        cols['median'][idx] = round(stats[2], 3)
        cols['mad'][idx] = round(stats[3], 3)
        cols['cv'][idx] = round(stats[4], 3)
        cols['MDE'][idx] = K * round(stats[3], 3)
        cols['min-delta'][idx] = K * round(stats[4], 3)

def write_md_table(f, title: str, table: dict[str, list]):
    df = pd.DataFrame.from_dict(table)
    tb = df.to_markdown(index=False)
    f.write(title)
    f.write(tb)
    f.write("\n")

def write_source(path: str):
    with open(path, 'w') as f:
        f.write(str(latest_latency_dir))
        f.write("\n")
        f.write(str(latest_perf_dir))