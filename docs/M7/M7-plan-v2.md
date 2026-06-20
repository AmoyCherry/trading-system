# M7 plan v2 — E2E benchmark suite that's sensitive

> **Current operational plan for M7.** Companion to [`M7-techniques.md`](M7-techniques.md) (conceptual reference) and [`M7-interview-prep.md`](M7-interview-prep.md) (pitch & Q&A).
>
> Distinctive choices in this plan:
> 1. Environmental hygiene applied + documented (§4)
> 2. Warmup elimination (§5)
> 3. Coordinated-omission addressed via open-loop generator (§6)
> 4. Microbench ↔ E2E cross-check of attribution model (§7)
> 5. P0 / P1 / P2 deliverable prioritization (§1, §12)
> 6. Sample stride 16 for attribution mode (§3)

---

## 0. What this milestone is and isn't

M7 is **measurement-validity infrastructure**, not optimization. The goal is to answer:

> *Is my benchmark scientifically valid enough that M8 optimization results will be believable?*

**Hard rule:** the correctness gate (`compare_3proc_hash.sh`) must pass on all 3 scenarios before any perf comparison. No exceptions.

---

## 1. Deliverables, prioritized

### P0 — must-have for `v4-e2e-measurement-baseline` tag

- `lobd --mode={null,match}` (decode mode is P1)
- Sampled timestamps in exchange / gateway / lobd, with throughput-mode (`--sample-stride 0`) and attribution-mode (`--sample-stride 16`) split
- `scripts/run_e2e_once.sh`
- `scripts/run_e2e_matrix.sh`
- `scripts/perf_stat_lobd.sh` — **verified working on this host before Week 1**
- `benchmarks/e2e/analysis/summarize_e2e.py`
- `docs/experiments/001_measurement_validity.md` populated with real numbers
- Environmental hygiene applied + documented
- Warmup spec applied + documented
- Microbench ↔ E2E cross-check executed

### P1 — add if Weeks 1–2 finish on time

- `lobd --mode=decode` (third attribution mode)
- `docs/experiments/002_workload_sensitivity.md`
- Full sampled-latency CSV export (per-message records, not just precomputed percentiles)

### P2 — first to drop if Week 3 slips

- `--inject-work-iters K` synthetic perturbation (positive control)

---

## 2. lobd modes

Three modes that do progressively more work — see [`M7-techniques.md` §4.1](M7-techniques.md#41-wire-to-wire-stage-attribution-lobd-modes--m7) for the conceptual story and the attribution arithmetic. Implementation notes specific to this codebase:

- Gate mode behavior with a `Mode` enum, branch hoisted out of the recv loop. **No virtual functions**, no inheritance — the dispatch overhead would muddy the very thing we're measuring.
- **`match - null` is the primary attribution claim** (engine + decode cost). `match - decode` and `decode - null` are *secondary*, documented as approximations. If decode mode is dropped (P1), the rest of M7 still holds.
- All three modes count frames and stop on `EndOfReplay`; only `match` updates the engine and emits `state_hash`.

---

## 3. Run types — two distinct workflows

| Run type | `--sample-stride` | Used for | Reported metrics |
|---|---|---|---|
| **Throughput** | `0` (off) | headline throughput, `perf stat` runs, M8 final claims | msgs/sec, cycles/msg, IPC, branch-miss-rate, cache-miss-rate |
| **Attribution** | `16` (or `64`) | stage-by-stage latency breakdown | p50 / p90 / p99 per stage |

**Never mix.** A throughput claim from an attribution-mode run is invalid — the timestamping perturbs the hot loop.

**p99.9 is throughput-mode only.** With stride 16 on N=1M (~62.5k samples), the p99.9 bucket has ~62 samples — usable. With stride 64 it's ~16 samples — too few. Default to stride 16; the per-message `CLOCK_MONOTONIC_RAW` cost (~20–30 ns) at 1/16 sampling is ≤ 0.2% of typical engine cycles/msg, so the overhead is acceptable.

---

## 4. Environmental hygiene (NEW)

Before any measurement run, declare which of these are applied. `001_measurement_validity.md` lists the host's actual state.

| Setting | Effect | Status target |
|---|---|---|
| `cpupower frequency-set -g performance` | Lock CPU frequency | **Required** |
| `echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo` | Disable Turbo Boost | **Required** |
| `kernel.perf_event_paranoid ≤ 2` | Allow `perf stat` hardware counters | **Required for perf** |
| `/proc/sys/kernel/randomize_va_space = 0` | Disable ASLR for reproducibility | Recommended |
| `isolcpus=`, `nohz_full=`, `rcu_nocbs=` on kernel cmdline | Dedicate measurement cores | Nice-to-have; declare absence if not set |
| Stop unrelated services (Slack, browsers, IDE indexers) | Reduce scheduler noise | **Required for measurement runs** |

**Action: verify perf counters NOW (before Week 1):**

```bash
perf stat -e cycles,instructions,branches,branch-misses,\
cache-references,cache-misses,LLC-loads,dTLB-load-misses /bin/true
```

If any counter is unavailable (common in VMs/containers/restricted hosts), you need to know before M8 case-study planning, not during.

---

## 5. Warmup (NEW)

Engine cold-start eats the first ~10k messages (cold I-cache, cold branch predictor, cold TLB on the order-book hashmap). Spec:

- Each run begins with `--warmup N` messages that are **not timestamped** and **not counted** toward throughput.
- For N=1M scenarios, use `--warmup 10000`.
- The analyzer additionally drops the first 1% of attribution samples as belt-and-suspenders.
- `summary.json` records the warmup count so the analyzer subtracts correctly.

---

## 6. Coordinated omission (NEW)

The exchange must generate **open-loop** — paced by an internal counter, not by gateway acks — so that lobd back-pressure does not silently truncate the tail. The current M6 generator already does this; verify and document.

State explicitly in `001_measurement_validity.md`:

> Exchange uses open-loop send (paced internally, not by gateway ack), so receiver slowdown does not coordinate-omit worst-case latencies. If lobd cannot keep up, we observe `seq_errors` / drops, not artificially-low p99.

Reference: Gil Tene, *How NOT to Measure Latency* (Strange Loop 2015).

---

## 7. Microbench ↔ E2E cross-check (NEW)

End-of-Week-2 sanity check, ~10 lines in the analyzer:

> The cycles/msg attributed to engine via E2E subtraction (`match cycles/msg − null cycles/msg`) should agree with microbench `BM_AddOrder` cycles/op within ~30% for `add_only`.

If the gap is > 2×, the attribution model is leaking (likely: cache state differs between modes, or decode-stripped recv loop has different inlining). Investigate before claiming any M8 case-study delta.

---

## 8. Scenarios

Three scenarios: `add_only`, `cross_mixed`, `cancel_heavy`. Each stresses a different code path (insert / matching / cancel) — see `002_workload_sensitivity.md` once written. `burst` is out of M7 scope (deferred to M8 if useful).

---

## 9. Noise floor & effect-size threshold

See [`M7-techniques.md` §4.3](M7-techniques.md#43-noise-floor--minimum-detectable-effect--m7) for the conceptual treatment.

Run config: `cross_mixed, mode=match, N=1M, sample_stride=0, pinned`, repeat **×10**. Compute:

- median msgs/sec
- MAD and stddev
- **coefficient of variation** (CV = stddev / median)

**Noise floor decision rule:** declare an optimization meaningful only if its effect exceeds `k × MAD`. Document k in the validity card; typical choice is **k = 3** (~3σ-equivalent).

**Targets at `v4` tag:** CV < 5% for throughput, CV < 10% for p99. If higher, revisit environmental hygiene before continuing — variance from frequency scaling / IRQs / context switches is fixable and should be fixed before optimization claims.

---

## 10. Synthetic perturbation (positive control) — P2

Add `--inject-work-iters K` to lobd apply path. Validate at K=0/16/64 that the harness detects the artificial slowdown. See [`M7-techniques.md` §4.4](M7-techniques.md#44-positive-control-via-synthetic-perturbation--m7) for why this matters.

**Industry phrasing for the validity card (and interviews):** *sanity check — if I inject a known ~1 µs delay, does the harness catch it?*

P2 priority: first to drop if Week 3 is tight.

---

## 11. Scripts

Four scripts: `run_e2e_once.sh`, `run_e2e_matrix.sh`, `perf_stat_lobd.sh`, `summarize_e2e.py`. Two operational notes:

- **`perf_stat_lobd.sh`** attaches to lobd via `perf stat -p <PID>` after the READY-line handshake. Reuse the polling helper from `run_e2e_3proc.sh`.
- **`summarize_e2e.py`** joins per-process CSVs by `seq`. Since stride is uniform across processes (every process samples `seq % stride == 0`), the join is trivial.

---

## 12. Schedule (re-prioritized)

Hard deadline: **tag `v4-e2e-measurement-baseline` no later than end-of-Week-3.** If incomplete, tag what works and move to M8 anyway — interview value of M7 plateaus once the validity card has defensible numbers.

### Week 1 — P0 core

- `lobd --mode=null` and `--mode=match`
- Sampled timestamps struct + buffers + stride flag in all 3 processes
- `run_e2e_once.sh`
- `summary.json` output per process
- Smoke test at N=200k, single scenario, single mode
- Environmental hygiene applied + documented
- Perf counter availability verified on this host

### Week 2 — P0 matrix + analyzer

- `run_e2e_matrix.sh`
- Analyzer (joins per-process CSVs, computes percentiles + variance)
- `perf_stat_lobd.sh`
- Run full matrix at N=1M with 5 repeats
- Microbench ↔ E2E cross-check executed
- *If on time:* add `--mode=decode` (P1)

### Week 3 — P0 validity card + polish

- `001_measurement_validity.md` populated with **real numbers** for: hygiene state, run-to-run CV, noise floor threshold, warmup spec, coordinated-omission justification, microbench cross-check result
- *If on time:* `002_workload_sensitivity.md`
- *If on time:* `--inject-work-iters` synthetic perturbation (P2)
- README polish: one M7 headline table

---

## 13. Definition of Done

M7 is complete when:

1. **Correctness gate passes** for all 3 scenarios at N=200k:
   ```bash
   scripts/compare_3proc_hash.sh add_only      200000
   scripts/compare_3proc_hash.sh cross_mixed   200000
   scripts/compare_3proc_hash.sh cancel_heavy  200000
   ```

2. `scripts/run_e2e_matrix.sh --n 1000000 --repeats 5` produces a full results directory.

3. `001_measurement_validity.md` lists, with real numbers:
   - Environmental hygiene state (governor, Turbo, paranoid, ASLR, isolcpus, services)
   - Run-to-run CV for throughput and p99
   - Noise floor threshold (k × MAD)
   - Warmup spec
   - Coordinated-omission justification
   - Microbench cross-check result

4. `match` mode state_hash equals `replay_ref` state_hash for matching `(scenario, N, seed)`.

5. `null < match` cost ordering holds, or has written explanation.

6. CV targets met (< 5% throughput, < 10% p99), or known-bad and documented.

7. Tag `v4-e2e-measurement-baseline`.

---

## 14. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Plan is large; M7 eats into M8 | Hard 3-week deadline. Tag and move on at end of Week 3 regardless. |
| Variance too high to detect 5% effects | Apply full environmental hygiene; if still high, document and lower MDE bar (but disclose). |
| `perf stat` counters unavailable on host | Verify Week 1, before downstream commitments. Worst case: report cycles/msg from clock-derived measurements, skip cache-miss / branch-miss attribution. |
| `decode` mode adds week of work for marginal value | Drop to P1; ship null+match in Week 1; add decode only if Week 2 is clean. |
| Synthetic perturbation reveals harness can't detect injected delays | Treat as a finding — investigate stride / warmup / timer resolution. Don't suppress; this is exactly what the positive control is for. |
