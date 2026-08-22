"""Same-run queue census for one transport hop (default: ex -> gw).

For each sampled message i, `outstanding(i)` = how many messages the producer
had sent that the consumer had not yet received at the moment i left the
producer (counts i itself, so 1 == arrived to an empty queue):

    outstanding(i) = i - #{j : recv[j] <= sent[i]}

Both timestamp columns are time-ordered (single-threaded processes, one
monotonic clock), so the count is one sorted lookup per message.

The drain cycle uses the MEAN gap between consecutive recv timestamps
(= 1 / same-run throughput): a queued message's wait is the SUM of the
cycles ahead of it, and sums pair with the mean (additive), not with the
median of a right-skewed cycle distribution. `q50_outstanding x mean_cycle`
then reconstructs the median wait because the depth barely varies at
saturation. Everything comes from ONE run — no cross-run lambda/W mixing.

Usage:
    .venv/bin/python scripts/stat/queue_census.py <dir> [--hop ex2gw|gw2lob]
        [--scenario cross] [--mode match] [--repeats 1,2,3]

    .venv/bin/python scripts/stat/queue_census.py docs/experiments/w2w_ex2gw_bottleneck/blocking/3us-ctxsw

<dir> is either an experiment folder containing source.txt (first line =
latency run dir) or an artifacts/results/latency_* run dir itself.
"""

import argparse
from pathlib import Path

import numpy as np
import pandas as pd

# hop -> (sender csv, sender stamp after handing the msg to the kernel,
#         receiver csv, receiver stamp when recv returned it)
HOPS = {
    "ex2gw": ("exts", "ex_after_send", "gwts", "gw_recv"),
    "gw2lob": ("gwts", "gw_after_send", "lobts", "lob_recv"),
}


def resolve_run_dir(path: Path) -> Path:
    """Accept an experiment folder (with source.txt) or a raw latency run dir."""
    src = path / "source.txt"
    if src.exists():
        run = Path(src.read_text().splitlines()[0].strip())
        if not run.is_dir():
            raise FileNotFoundError(f"source.txt points to a missing run dir: {run}")
        return run
    if path.is_dir():
        return path
    raise FileNotFoundError(f"not a directory: {path}")


def list_repeats(base: Path) -> list[int]:
    reps = sorted(int(p.name.split("_")[1]) for p in base.glob("repeat_*"))
    if not reps:
        raise FileNotFoundError(f"no repeat_* dirs under {base}")
    return reps


def census_one(rep_dir: Path, hop: str) -> dict:
    send_csv, send_col, recv_csv, recv_col = HOPS[hop]
    snd = pd.read_csv(rep_dir / f"{send_csv}.csv", usecols=["seq", send_col])
    rcv = pd.read_csv(rep_dir / f"{recv_csv}.csv", usecols=["seq", recv_col])
    if snd.empty or rcv.empty:
        raise ValueError(
            f"no samples in {rep_dir} — null-mode / stride-0 runs carry no timestamps"
        )
    merged = snd.merge(rcv, on="seq", how="inner").sort_values("seq")
    if len(merged) != len(snd) or len(merged) != len(rcv):
        raise ValueError(
            f"seq mismatch in {rep_dir}: send={len(snd)} recv={len(rcv)} joined={len(merged)}"
        )
    sent = merged[send_col].to_numpy(np.int64)
    recv = merged[recv_col].to_numpy(np.int64)
    for name, ts in ((send_col, sent), (recv_col, recv)):
        if np.any(np.diff(ts) < 0):
            raise ValueError(
                f"{name} not monotonic in {rep_dir} — census assumes in-order, "
                "single-threaded timestamps on one clock"
            )

    n = len(merged)
    # sent-so-far (incl. self) minus received-so-far at each send moment
    outstanding = np.arange(1, n + 1) - np.searchsorted(recv, sent, side="right")
    q50_out = float(np.median(outstanding))
    mean_cycle = (recv[-1] - recv[0]) / (n - 1)
    # depth x cycle reconstructs the median wait only when a queue exists;
    # with q50 <= 1 the "cycle" is just the pacing gap, so blank it out
    recon = float(q50_out * mean_cycle) if q50_out > 1 else np.nan
    return {
        "n": n,
        "hop_p50_ns": float(np.median(recv - sent)),
        "q50_outstanding": q50_out,
        "frac_alone": float(np.mean(outstanding <= 1)),
        "drain_cycle_mean_ns": float(mean_cycle),
        "recon_q50xcycle_ns": recon,
        "send_rate_k_per_s": float((n - 1) / (sent[-1] - sent[0]) * 1e6),
        "drain_rate_k_per_s": float((n - 1) / (recv[-1] - recv[0]) * 1e6),
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("dir", type=Path, help="experiment folder (with source.txt) or latency run dir")
    ap.add_argument("--hop", choices=HOPS, default="ex2gw")
    ap.add_argument("--scenario", default="cross")
    ap.add_argument("--mode", default="match")
    ap.add_argument("--repeats", default="all", help="comma list, e.g. 1,2,3 (default: all)")
    args = ap.parse_args()

    run_dir = resolve_run_dir(args.dir)
    base = run_dir / args.scenario / args.mode
    reps = list_repeats(base) if args.repeats == "all" else [int(r) for r in args.repeats.split(",")]

    rows = {rep: census_one(base / f"repeat_{rep}", args.hop) for rep in reps}
    df = pd.DataFrame.from_dict(rows, orient="index")
    df.index.name = "repeat"
    df.loc["median"] = df.median()

    print(f"# queue census — {run_dir.name} / {args.scenario} / {args.mode} / hop={args.hop}")
    out = df.round(
        {
            "n": 0,
            "hop_p50_ns": 0,
            "q50_outstanding": 1,
            "frac_alone": 3,
            "drain_cycle_mean_ns": 1,
            "recon_q50xcycle_ns": 0,
            "send_rate_k_per_s": 1,
            "drain_rate_k_per_s": 1,
        }
    )
    try:
        print(out.to_markdown())
    except ImportError:  # tabulate not installed
        print(out.to_string())


if __name__ == "__main__":
    main()
