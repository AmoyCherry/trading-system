import math

import pandas as pd
import numpy as np
import json
from dataclasses import fields
from pathlib import Path
from scipy.stats import bootstrap
import re

from scripts.stat.models import RepeatIntervals, RepeatIntervalMetrics, PerfCell, LatencyCell, CounterMetrics, Stats

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

UNBIASED_RobustCV_FACTOR = 1.4826 * 1.039

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

    assert ex_seq == gw_seq == lob_seq, f"seq match failure\n s:{scenario} m:{mode}, r:{repeat}\n ex_seq len: {len(ex_seq)}, gw_seq len: {len(gw_seq)},  lob_seq len, {len(lob_seq)}"
    assert_intervals_non_negative(intervals, scenario, mode, repeat)

    return intervals

def assert_intervals_non_negative(intervals: RepeatIntervals, scenario, mode, repeat):
    for f in fields(intervals):
        arr = getattr(intervals, f.name)
        try:
            assert all(x >= 0 for x in arr), f"{scenario}, {mode}, {repeat}, {f.name} has negatives."
        except:
            print(f"{scenario}, {mode}, {repeat}, {f.name} has negatives.")

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

def robust_cv(arr):
    return UNBIASED_RobustCV_FACTOR * mad(arr) / np.median(arr)

def stat_latency_repeats(latency_metrics_repeats: list[RepeatIntervalMetrics]) -> LatencyCell:
    df = pd.DataFrame(latency_metrics_repeats)
    summary_stats = df.agg(['mean', 'std', 'median', mad, robust_cv])

    # summary_stats.to_dict() produces a nested dictionary:
    # {'ex_intvl_send_p50': {'mean': 10.5, 'std': 1.2, 'median': 10.3, 'mad': 0.8, 'robust_cv': 0.1}, ...}
    cell_kwargs = {
        f"{col}_stat": Stats(**stats_dict)
        for col, stats_dict in summary_stats.to_dict().items()
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


    # rusage
    def parse_ru(log_path: str, key:str) -> int:
        pattern = re.compile(rf"\b{re.escape(key)}=(\d+)")
        with open(log_path, 'r') as f:
            for line in f:
                match = pattern.search(line)
                if match:
                    return int(match.group(1))
        raise ValueError(f"No {key} found in {log_path}")

    ex_latency_path = get_log_path(latest_latency_dir, scenario, "match", repeat, "exchange.out")
    lob_latency_path = get_log_path(latest_latency_dir, scenario, "match", repeat, "lobd.log")
    gw_latency_path = get_log_path(latest_latency_dir, scenario, "match", repeat, "gateway.log")

    # No default val to destroy results, fail loud!
    return CounterMetrics(
        ipc=safe_div(raw_data['instructions'], raw_data['cycles']),
        cycle_per_msg=safe_div(raw_data['cycles'], msgs),
        cache_miss_rate=safe_div(raw_data['cache_misses'], raw_data['cache_references']),
        branch_miss_rate=safe_div(raw_data['branch_misses'], raw_data['branches']),
        throughput=float(throughput),
        ex_vol_ctx_sw=parse_ru(ex_latency_path, "vol_ctx_sw"),
        ex_invol_ctx_sw=parse_ru(ex_latency_path, "invol_ctx_sw"),
        gw_vol_ctx_sw=parse_ru(gw_latency_path, "vol_ctx_sw"),
        gw_invol_ctx_sw=parse_ru(gw_latency_path, "invol_ctx_sw"),
        lob_vol_ctx_sw=parse_ru(lob_latency_path, "vol_ctx_sw"),
        lob_invol_ctx_sw=parse_ru(lob_latency_path, "invol_ctx_sw"),
    )

def stat_perf_repeats(counters_repeats: list[CounterMetrics]) -> PerfCell:
    df = pd.DataFrame(counters_repeats)
    summary_stats = df.agg(['mean', 'std', 'median', mad, robust_cv])

    cell_kwargs = {
        f"{col}_stat": Stats(**stats_dict)
        for col, stats_dict in summary_stats.to_dict().items()
    }

    return PerfCell(**cell_kwargs)


# ------------- summary -----------------------------------------
def med_mad(stats: Stats, div = 1) -> str:
    # ['mean', 'std', 'median', mad, cv]
    return f"{(stats.median / div):.2f} ± {(stats.mad / div):.2f}"

def median_delta_with_ci(metric_name: str, metrics_repeats_l: list[CounterMetrics], metrics_repeats_s: list[CounterMetrics], ci_lvl = 0.95) -> str:
    metric_arr_l = [getattr(metrics, metric_name) for metrics in metrics_repeats_l]
    metric_arr_s = [getattr(metrics, metric_name) for metrics in metrics_repeats_s]
    actual_delta = np.median(metric_arr_l) - np.median(metric_arr_s)

    SEED = 97
    rng = np.random.default_rng(SEED)
    delta_lbd = lambda l, s: np.median(l) - np.median(s)
    res = bootstrap((metric_arr_l, metric_arr_s),
                    delta_lbd,
                    rng=rng,
                    confidence_level=ci_lvl)

    return f"{actual_delta:.2f}, [{res.confidence_interval.low:.2f}, {res.confidence_interval.high:.2f}]"

def decomp(cols: dict[str, list], latency_cell: LatencyCell):
    # ['mean', 'std', 'median', mad, cv]
    w2w_mean_med = latency_cell.w2w_mean_stat.median
    for idx, metric_name in enumerate(cols['metric']):
        stats = getattr(latency_cell, metric_name)
        cols['median (ns)'][idx] = f"{stats.median}"
        cols['mad (ns)'][idx] = round(stats.mad, 3)
        cols['robust cv'][idx] = round(stats.robust_cv, 3)
        cols['%w2w'][idx] = f"{(stats.median / w2w_mean_med):.2%}"

K = 3
def estimate_gate_noise(cols: dict[str, list], perf_metrics: list[str], latency_cell: LatencyCell, perf_cell: PerfCell):
    for idx, metric_name in enumerate(cols['metric']):
        if metric_name in perf_metrics:
            stats = getattr(perf_cell, metric_name)
        else:
            stats = getattr(latency_cell, metric_name)

        cols['median'][idx] = round(stats.median, 3)
        cols['mad'][idx] = round(stats.mad, 3)
        cols['robust cv'][idx] = round(stats.robust_cv, 3)
        cols['MDE'][idx] = K * round(stats.mad, 3)
        cols['min-delta'][idx] = K * round(stats.robust_cv, 3)

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

HEADLINE_DOC = """
The stats data presented in `center ± spread` is `median ± mad`.
> Two families, `mean-std-cv` and `med-mad-robustc cv`, to stat (center ± spread) and data variance.
> We (me) choose the `med` family because:
> 1. `Mean` is more sensible to "bad tails" and "spikes" produced by corrupted runs, especially in a small sample. While `median` has 50% *Breakdown Point* to tolerate these corrupted runs data points.
> 2. `mean` is much efficient when the data is a clear Gaussian distribution. While with 10, 20 or 30 data points, we can not find a symmetric bell curve.

**`engine cyc/msg`**

Cross-mode delta is calculated by `med(match) - med(decode)` and is guarded by **CI**. The CI is calculated by  **bootstrapping**.

To calculate the deltas between two exprtl sets per counter, we must go with CI to calculate the uncertainty.

- The above two arguments about `mean` is still valid here. So we continue with median for deltas.
- Hodges-Lehmann CI is a CI for differences between two sets, it calculates the diff of every possible elem pair between two sets to get a `n * n` difference array, then return the median. But it assumes that the two sets have the identical shape and only different in location shift. With 10, 20 or 30 samples, it's even hard to define a shape.
"""

DECOMP_DOC = """
w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
"""

NOISE_DOC = """
> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
"""