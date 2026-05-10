# Case Study Card — Template

A perf claim without these fields is not reproducible and not defensible in an interview. Copy this file to `00X_<short_name>.md` per experiment.

## Scientific core (why each field exists)

- **Single variable.** Change one thing per card. If two changes ship together, you cannot attribute the delta — the card is worthless.
- **Baseline-anchored.** Every number is "vs. commit `<sha>` / tag `<v0-...>`". "Faster" without a baseline is folklore.
- **Hypothesis before measurement.** Predict which counter moves and why. Predictions that fail are the most informative results — they invalidate your mental model and that is a finding.
- **Workload is part of the result.** Latency distribution is a function of input distribution. Same code with a different mix gives a different number; reporting metrics without the workload is nonsense.
- **Three metric families.** Throughput, latency percentiles, and 1–2 perf counters. Throughput alone hides tail regressions; p99 alone misses average-case effects; counters alone miss user-visible behavior. The triple is the minimum.
- **Counter–hypothesis match.** If you hypothesized "fewer L1 misses" and latency dropped but L1 misses did not — your causal story is wrong, even if the change is good. Note it.
- **Statistical hygiene.** Report median (or min) over ≥5 repeats, not a single run. Single-run perf numbers are noise.
- **Environment is fixed.** Governor, Turbo, SMT, pinning, ASLR pinned across before/after. If any drift, the comparison is invalid.
- **Threats to validity.** Name the one or two things that, if true, would invalidate this card. Forces honest thinking and pre-empts the interview follow-up question.

## What we actually measure

| Layer | Tool | Question it answers |
|---|---|---|
| Engine hot path | `inproc_runner` (no IPC noise) | What is engine-only p50/p99? |
| 3-proc lobd stage | per-process timestamp log + `percentiles.hpp` | What does engine cost in the real pipeline? |
| 3-proc end-to-end | timestamps at exch.send / gw / lob, joined by seq | Where does total RTT go? |
| Hardware | `perf stat -p $LOB_PID` | Did cycles / cache / branch behavior change? |
| Determinism | `replay_ref` and 3-proc state hash | Are we comparing identical engine state? |

`null_engine` mode (lobd ACKs without matching) lets you subtract transport from total to isolate engine cost.

---

## Template (copy below this line)

# 00X — `<short title>`

**Date:** YYYY-MM-DD
**Author:** <name>
**Baseline:** tag `<v0-baseline-3proc>` / commit `<sha>`
**Comparison:** commit `<sha>`
**Machine:** CPU model, kernel, compiler+flags, RAM, single-/dual-socket

## Hypothesis
One sentence. The form: *"Changing X will move metric M by direction D because mechanism Y."*
Example: *"Replacing `std::map<Price, Level>` with a sorted vector of (price, level) will reduce lobd p99 because best-price lookup becomes a single load instead of an O(log N) tree walk with cache-unfriendly node layout."*

## Change
1–3 lines. Link to the diff/PR.

## Workload
- Scenario(s): `add_only` / `cancel_heavy` / `aggressive_cross` / `burst`
- Size N: …
- Seed: …
- Warmup: … repeats: … (median reported)
- Mode: `null_engine` / `match_engine` / `inproc`

## Environment
- Governor: `performance` · Turbo: off · SMT: off (or pinned to one sibling)
- Pinning: lobd→core X, gateway→core Y, exchange→core Z (`taskset` or in-binary)
- ASLR: off · `isolcpus`: yes/no
- Build: `RelWithDebInfo`, `-O2 -g`, LTO on/off

## Metrics

| metric | before | after | Δ | Δ% |
|---|---|---|---|---|
| throughput (msg/s) | | | | |
| p50 (ns) | | | | |
| p99 (ns) | | | | |
| p99.9 (ns) | | | | |
| max (ns) | | | | |
| cycles / op | | | | |
| instructions / op | | | | |
| L1-dcache-load-misses | | | | |
| branch-misses | | | | |
| context-switches | | | | |

(Pick 1–2 perf counters that map to the hypothesis. Don't dump all of them.)

## Interpretation
- Did the predicted counter move in the predicted direction? By how much?
- Does the magnitude match the latency delta? (E.g., 30 fewer cache misses ≈ 30·~10 ns ≈ 300 ns saved.)
- If counter and latency disagree, what's the alternative explanation?

## Decision
**Keep · Revert · Follow-up needed**
Reason in one line.

## Threats to validity
1–2 items. Examples: "single repeat per workload", "baseline used different governor setting", "noise floor not measured".

## Follow-ups
1–2 next experiments this opens. Often: a workload where this should *fail*, or a related layer to apply the same technique.
