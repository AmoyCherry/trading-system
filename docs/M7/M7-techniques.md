# M7 techniques — measurement & attribution

> Conceptual reference for the high-ROI techniques used in M7. Read for *understanding*, not *rehearsal*.
> Companion to [`M7-plan-v2.md`](M7-plan-v2.md) (operational checklist) and [`M7-interview-prep.md`](M7-interview-prep.md) (pitch & Q&A).
>
> **Narrative arc:** clean the environment → eliminate artifacts → attribute → quantify residual noise → diagnose mechanism. Each section builds on the previous.
>
> **Legend:** `[M6]` = already shipped, articulate at PhD depth. `[M7]` = new in M7. `[X]` = cross-cutting.

---

## §1. Foundations — reproducibility before measurement

### 1.1 Deterministic replay with state-hash equality  `[M6]`

**What.** Given the same input trace and seed, the engine produces the same output and the same internal state, bit-exact. Verified by FNV-1a hashing the live order book at end of run and comparing between an in-process reference and the 3-process pipeline.

**Why it matters.**
- A benchmark you can't reproduce is a benchmark you can't trust. The first failure mode of perf work is "I ran it twice and got different numbers" — not because of variance, but because of non-determinism (allocator addresses, iteration order over unordered containers, race conditions in IPC, signal delivery timing).
- The state hash catches a *whole class* of bugs that pass functional tests: an off-by-one in level erase, a wrong-side fill emission, an iterator invalidation that *happens* to produce the same fills but a different book.
- Cross-path hash equality (`replay_ref` ↔ 3-process) verifies that the *transport layer* doesn't perturb engine semantics. Without this, you can't say "performance differences across modes come from cost, not from behavior."

**How (in this project).**
- `BookSummary::state_hash` is FNV-1a over `(live_order_count, every price level, every order's full state)` with side-prefix bytes (`'B'`, `'A'`) to prevent bid/ask collision. See `src/engine/book/order_book.cpp:215-262`.
- `scripts/compare_3proc_hash.sh` runs `replay_ref` in-process and the 3-proc pipeline on the same `(scenario, N)`, greps `state_hash=`, asserts equality.
- **Hard rule:** M7 refuses to run perf comparisons if this gate fails.

**Signal value.** Tier-S at Jane Street / Two Sigma. Strong signal at HRT / Citadel / Jump / Optiver. Almost no student project has this — leading with it in an interview is a near-guaranteed differentiator.

**Read more.** Jane Street *Signals & Threads* episodes on production-replay; the general idea is described in any text on "record-and-replay debugging."

---

### 1.2 `CLOCK_MONOTONIC_RAW` for interval measurement  `[M6]`

**What.** A POSIX clock that is monotonic and *not* subject to NTP slewing, `adjtime` adjustments, or wall-clock changes. Tick rate is the raw hardware TSC (modulo virtualization).

**Why it matters.**
- `CLOCK_REALTIME` jumps when NTP corrects the clock — you can record a negative interval. Don't use it for latency.
- `CLOCK_MONOTONIC` is monotonic but *is* slewed by NTP. For ~ms intervals that's fine; for ~µs/ns latency it's a contaminant.
- `CLOCK_MONOTONIC_RAW` is what kernel perf measurement code paths use. It's the right default for "how long did this take?"

**How (in this project).** `src/common/time/clock.cpp:8` uses `CLOCK_MONOTONIC_RAW` for all timestamping. Don't change this without reason.

**Caveat to know.** `clock_gettime(CLOCK_MONOTONIC_RAW, ...)` is a vDSO call on modern Linux, so it doesn't trap to the kernel — but it's still ~20–30 ns. That's why **§3.2 (throughput vs attribution mode split)** exists.

**Signal value.** Tier-A — interviewers expect you to know this; not knowing it is a *negative* signal.

---

### 1.3 CPU pinning (two-mechanism story)  `[M6]`

**What.** Bind each process to a specific CPU core so it doesn't migrate. Either externally via `taskset -c N` or in-binary via `sched_setaffinity`.

**Why it matters.**
- Cross-core migration invalidates L1/L2 caches, causes branch-predictor retraining, and adds ~µs of jitter per migration. For ns-scale measurements this is catastrophic.
- Three processes sharing one core means kernel context-switches them on every timeslice; that adds *tens* of µs of jitter to p99.
- The two-mechanism story matters because they interact: if both are active, the in-binary call can *widen* taskset's mask silently. Detecting this requires checking `/proc/<pid>/status` `Cpus_allowed`.

**How (in this project).**
- `--cpu-core -1` sentinel in lobd/gateway/exchange means "skip in-binary pinning"; this lets `taskset` own the policy.
- `scripts/run_e2e_3proc.sh` defaults to taskset; flag `--cpu-core` switches to in-binary.
- `log_affinity()` in the runner verifies via `taskset -p <PID>` that the kernel actually applied the mask. Don't skip this — cgroups can silently override.

**Signal value.** Tier-A. The *story* (chose to implement both, planned a measurement to compare) is the differentiator; pinning by itself is table stakes.

**Read more.** `man 2 sched_setaffinity`; `man 1 taskset`. Brendan Gregg's *Systems Performance* §6 on CPU affinity.

---

## §2. Suppress controllable noise

### 2.1 Environmental hygiene  `[M7]`

**What.** A checklist of host-level settings that introduce variance if left at defaults. Apply them *before* measuring, declare which are applied in `001_measurement_validity.md`.

**Why it matters.**
- **CPU governor.** `ondemand` / `schedutil` (the default) ramps frequency in response to load. Your N=1M run takes ~1 s; in that second the governor's decision can flip 5–10×, making run-to-run timing differ by 20–30%. `performance` governor locks the highest steady-state frequency.
- **Turbo Boost.** Boost frequency depends on temperature and on how many cores are active — i.e., on what *else* the machine is doing. Run twice, get two boost trajectories, get two different timings. Disable for measurement.
- **`perf_event_paranoid`.** If > 2, hardware counters (cache-misses, branch-misses) are unavailable to non-root. You don't know this until you try.
- **ASLR.** Different address layout → different `unordered_map` bucket layout → different cache lines hit → different miss rates. Small effect (1-3%) but real for ns-scale comparisons.
- **`isolcpus=` / `nohz_full=` / `rcu_nocbs=`.** Tell the kernel "don't schedule anything else on these cores, don't deliver the scheduler tick, don't run RCU callbacks." Strongest single win for tail latency, but requires kernel cmdline edit + reboot.
- **Background services.** Slack, browsers, IDE indexers, antivirus, log shippers — anything that wakes up every few seconds steals cycles and pollutes caches. Stop them for measurement runs.

**How (in this project).** Before any matrix run:

```bash
sudo cpupower frequency-set -g performance
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
sudo sysctl kernel.perf_event_paranoid=1
sudo sysctl kernel.randomize_va_space=0
# (optional, requires reboot) isolcpus=2,3,4 nohz_full=2,3,4 rcu_nocbs=2,3,4
# (manual) close Slack, browsers, IDE
```

Document the applied state at run time in `summary.json`.

**Signal value.** Tier-A. Lacking the *vocabulary* (governor / Turbo / paranoid / isolcpus) is a negative signal at HRT/Jump/Citadel.

**Read more.** Linux Kernel Documentation `Documentation/admin-guide/pm/cpufreq.rst`; `Documentation/admin-guide/kernel-per-CPU-kthreads.rst`.

---

### 2.2 Warmup elimination  `[M7]`

**What.** Drop the first ~N messages of a run from timing because they pay startup costs that don't represent steady state.

**Why it matters.** The first ~1k–10k messages hit:
- **Cold instruction cache** — the hot loop hasn't been fetched into L1i yet. Several hundred ns extra per message.
- **Cold branch predictor** — the BPU has no history yet; mispredict rate is near 50% for the first few iterations and decays to near-baseline over ~hundreds of branches per branch site.
- **Cold data cache** — `unordered_map` buckets haven't been touched. First insert into a bucket → L1/L2 miss → potentially LLC miss → potentially page-walk + TLB miss.
- **Allocator warmup** — the first allocations trigger `mmap` syscalls; the allocator's internal free list isn't populated.
- **Page faults** — first write to a fresh page triggers a major or minor fault. ~1–2 µs each.

Including warmup in your average crushes both your throughput claim and your tail latency. Including it in p99 is *worse* — those startup samples land in the tail and look like real latency events.

**How (in this project).**
- `--warmup N` flag on lobd: receive and process N messages, but do not record timestamps or count toward throughput.
- For N=1M scenarios, use `--warmup 10000`.
- Analyzer additionally drops the first 1% of attribution-mode samples as belt-and-suspenders.
- `summary.json` records `warmup_count` so post-hoc analysis can subtract correctly.

**Signal value.** Tier-A. The concept "warmup" is universal in production benchmarking; not handling it explicitly looks naive.

**Read more.** Andrei Alexandrescu, *Writing Quick Code in C++, Quickly* (CppCon 2017); JMH (Java Microbenchmark Harness) documentation on warmup iterations — the JMH model is the de facto standard.

---

## §3. Avoid artifacts in the measurement itself

### 3.1 Open-loop generation & coordinated omission  `[M7]`

**What.** Coordinated omission = when the *producer* slows down because the *consumer* is slow, so the worst-case latencies are never recorded — they get coordinated-out of the histogram. The fix is **open-loop generation**: the producer paces itself by an internal counter, not by the consumer's acknowledgments.

**Why it matters.** Imagine your generator sends a message every 1 µs in a tight loop, but lobd takes 100 µs to process one of them. In a *closed-loop* generator (one that waits for ack before sending next), the generator stalls for 99 µs — and never sends the messages that would have arrived during that stall. Your latency histogram contains exactly one 100-µs sample, instead of the 100 stalled-from-1-µs-to-100-µs samples that *would* have been recorded had you been sending at the advertised rate. Your p99 looks fine. Your benchmark is lying.

This is *the* canonical pitfall of latency measurement. Gil Tene named it; every serious latency tool (HdrHistogram, wrk2, rust's `hdrhistogram`) has explicit coordinated-omission handling.

**How (in this project).** Exchange's generator is open-loop by design: it iterates over the pre-generated message stream and sends as fast as it can, paced by nothing. If lobd back-pressures (via socket buffer full or via being slow), the symptom is **drops** (`seq_errors`) or **socket-buffer growth**, not artificially-good p99. State this explicitly in `001_measurement_validity.md`:

> Exchange uses open-loop send (paced internally, not by gateway ack), so receiver slowdown does not coordinate-omit worst-case latencies. If lobd cannot keep up, we observe `seq_errors` / drops, not artificially-low p99.

**Signal value.** Tier-S at Jane Street / Two Sigma; Tier-A elsewhere. Interviewers ask "what's coordinated omission?" specifically to filter for this — almost every senior infra interviewer at top quant shops knows the Gil Tene talk.

**Read more.** Gil Tene, *How NOT to Measure Latency* (Strange Loop 2015). **Mandatory viewing in Week 1.** Watch all 45 minutes; he covers more than just coordinated omission.

---

### 3.2 Throughput vs attribution mode separation  `[M7]`

**What.** Run the benchmark twice with the same workload but different instrumentation: one with no timestamping (for throughput claims) and one with sampled per-message timestamps (for stage-by-stage latency).

**Why it matters.**
- A `clock_gettime(CLOCK_MONOTONIC_RAW)` call costs ~20–30 ns. Cheap, but not free.
- If your hot path is ~200 cycles/msg ≈ 70 ns/msg, then a single per-message timestamp adds 25–40% overhead. *Six* per-message timestamps (the design in M7) would more than 2× the engine cost.
- This means: a throughput claim made *with* timestamping is not the same number as a throughput claim made *without*. They measure different systems.
- Sampling (every 16th or every 64th message) reduces the overhead proportionally but doesn't eliminate the problem — and the sampled subset is biased toward "messages that happened at sample points," which may correlate with workload phase.

**How (in this project).**
- `--sample-stride 0` → throughput mode. No timestamps recorded. Use for: msgs/sec, `perf stat` runs, M8 final claims.
- `--sample-stride 16` → attribution mode. Sample every 16th message. Use for: p50/p90/p99 per stage.
- **Never mix.** A throughput claim from an attribution-mode run is invalid.
- p99.9 is throughput-mode only (attribution mode has ~62 samples in the p99.9 bucket, too few).

**Signal value.** Tier-A. The *meta* point — "instrumentation perturbs measurement" — is what lands. Most students don't separate these run types.

**Read more.** Heisenberg uncertainty as analogy is over-cute; the real reference is the JMH wisdom that "if you don't disable JIT inlining for your @Benchmark methods, you're not measuring what you think." Same principle, different language.

---

## §4. Attribute the cost, then quantify residual noise

### 4.1 Wire-to-wire stage attribution (lobd modes)  `[M7]`

**What.** lobd runs in one of three modes, each a strict superset of the previous. Cost differences between modes attribute latency to specific pipeline stages.

| Mode | Work | Floor |
|---|---|---|
| `null` | header validation only | transport + recv-loop |
| `decode` | + full wire decode | + codec |
| `match` | + engine apply + state hash | + engine |

Attribution arithmetic: `null` ≈ transport + recv-loop; `decode − null` ≈ codec; `match − decode` ≈ engine.

**Why it matters.** Without decomposition, a `match` throughput delta is unattributable — engine, codec, gateway scheduling, and OS noise are indistinguishable. With decomposition, a delta isolated to `match` (with null and decode unchanged) and concentrated on `cancel_heavy` is consistent with a cancel-path mechanism; competing explanations are ruled out by data, not argument.

**Limits.** Cache, branch-predictor, and TLB state carry across modes — match warms the engine icache differently than null, so `match − decode` absorbs cache-state divergence in addition to the work itself. The decomposition is an attribution model, not a causal proof. Cross-check engine cost against microbench cycles/op (§4.2).

**How (in this project).** Mode is selected at startup via `--mode {null,decode,match}`. A runtime switch in `main()` dispatches to a compile-time-specialized recv loop (`lobd_main<Mode>()`); nested `if constexpr` resolves all mode logic at compile time. Result: zero per-iteration dispatch, three independent I-cache footprints, full inlining per specialization. See [`M7-plan-v2.md` §2](M7-plan-v2.md#2-lobd-modes).

**Alternatives considered.** A plain `if`/`else` in the hot loop would predict perfectly (stationary branch, ~0 mispredicts after iteration 1–2; cost is the 1–2 instructions to evaluate, not mispredict noise) but shares an I-cache footprint across all three modes and prevents per-mode dead-code elimination. `std::variant` + `std::visit` with visit wrapping the loop is equivalent in dispatch cost to the template approach but forces shared scaffolding (recv, header decode, EndOfReplay check) to be split across per-mode lambdas. Virtual dispatch has the same source-duplication issue and additionally prevents inlining.

**Signal value.** Quant infra interviews probe stage attribution directly ("what's your transport cost vs engine cost?"). Industry vocabulary: *wire-to-wire decomposition*, *tick-to-trade breakdown*, *stage timestamps*.

**Read more.** Carl Cook, *When a Microsecond Is an Eternity* (CppCon 2017) — Optiver tick-to-trade pipeline.

---

### 4.2 Microbench ↔ E2E cross-check  `[M7]`

**What.** Verify that the cycles/msg attributed to "engine" via E2E subtraction agrees with the cycles/op measured directly by microbench. If they diverge by > 2×, the attribution model is leaking.

**Why it matters.** The subtraction `match - null` (or `match - decode`) is a *model*, and models lie. Specific ways it can lie:
- Different inlining decisions in the recv loop when match-mode vs null-mode is compiled (less likely if you used an enum; possible if you used templates).
- Cache state: match mode pulls engine code into L1i, displacing decode code; null mode doesn't, so its baseline cache state differs.
- Branch-predictor state: match mode trains the BPU on engine branches; null mode doesn't.

Microbench gives you a direct measurement of `BM_AddOrder` cycles/op in *isolation* — no recv loop, no mode flag. If your subtraction agrees, the model is sound. If it disagrees badly, you've discovered something — investigate before claiming an M8 case-study win.

**How (in this project).** End of Week 2:

```python
# in summarize_e2e.py or a separate script
engine_cycles_per_msg_e2e = match_cycles_per_msg - null_cycles_per_msg  # add_only scenario
engine_cycles_per_op_micro = bm_addorder_cycles_per_op
ratio = engine_cycles_per_msg_e2e / engine_cycles_per_op_micro
assert 0.7 < ratio < 1.4, "attribution model is leaking"
```

If the assertion fails, don't suppress — that's the finding. Investigate.

**Signal value.** Tier-A. The *idea* of cross-checking two independent measurements of the same quantity is universal in experimental science but rarely surfaced in CS interviews; calling it out makes you sound senior.

---

### 4.3 Noise floor & minimum detectable effect  `[M7]`

**What.** Run the same config (`cross_mixed, match, N=1M, sample_stride=0, pinned`) ten times. Compute median, MAD (median absolute deviation), and CV (coefficient of variation). Define a decision rule: declare an optimization meaningful only if its effect exceeds `k × MAD`.

**Why it matters.**
- Run-to-run variance has a floor, set by what you couldn't eliminate (OS scheduler, IRQs, residual frequency drift, microarchitectural noise). Below that floor, "improvements" are coin-flips.
- Without a quantified noise floor, every claim is ambiguous. "Got 4% faster" — was the median actually 4% better, or did you happen to catch a low run in the new build and a high run in the old?
- The MAD-based threshold is robust to outliers (unlike stddev). `k=3` is roughly the equivalent of "3σ for Gaussian data," meaning ~0.3% chance of false positive under stationary noise. Conservative; appropriate.

**How (in this project).**
- The 10× repeat config is documented in `M7-plan-v2.md` §9.
- Target CV at `v4` tag: < 5% for throughput, < 10% for p99. If higher, return to §2.1 (environmental hygiene) — variance from frequency scaling and scheduler noise is fixable and should be fixed before any optimization claim.
- State the threshold in `001_measurement_validity.md`: "Effect must exceed 3 × MAD of the baseline distribution to be claimed."

**Industry phrasing.** "Noise floor." "Run-to-run noise." "Smallest speedup I can claim isn't noise." Don't say "minimum detectable effect" in interviews — it's correct PhD vocabulary but reads as overengineered to most interviewers.

**Signal value.** Tier-S at Jane Street / Two Sigma. Tier-A elsewhere. Almost no student project does this; it's the single biggest "I think like a quant researcher" signal you can ship.

**Read more.** Gil Tene's *How NOT to Measure Latency* covers the statistical issues too. For deeper: any introductory experimental statistics text on confidence intervals and effect sizes.

---

### 4.4 Positive control via synthetic perturbation  `[M7]`

**What.** A debug flag (`--inject-work-iters K`) that adds K iterations of dummy work in the lobd apply path. Run the harness at K=0, 16, 64. If the harness can resolve the difference, it's sensitive enough for the noise floor. If it can't, the harness needs more work.

**Why it matters.** This is the *positive control* from experimental science.
- Negative controls (null mode) tell you "this is what zero work looks like." Useful but not sufficient.
- Positive controls (known-magnitude perturbation) tell you "this is what *known* work looks like — does your measurement reflect it?" If yes, your harness is calibrated. If no, all subsequent optimization claims are suspect.
- Particularly valuable because it catches subtle bugs: a timer call that's optimized out by the compiler, a sample buffer that's silently overflowing, a stride that's higher than you think because of an off-by-one.

**How (in this project).**
- Add `--inject-work-iters K` to lobd. Inside the apply path: `for (volatile int i = 0; i < K; ++i) {}` (the `volatile` prevents the compiler from optimizing it away).
- Run at K=0, 16, 64. Expected: throughput decreases monotonically; p99 increases monotonically. If a value of K produces an effect smaller than your noise floor, that's the resolution limit of your harness.
- Document in `001_measurement_validity.md` what the smallest detectable K is.

**Caveat.** `volatile int` is the simplest perturbation but its actual cycle cost depends on compiler/CPU. A more controlled alternative is `__builtin_ia32_pause()` (Intel PAUSE instruction, ~5-100 cycles depending on µarch) or `_mm_lfence()`. For M7 the `volatile` loop is sufficient; the exact cost doesn't matter as long as it's reproducible.

**Signal value.** PhD-flavored gold. Tier-S at Jane Street / Two Sigma. Tier-A at HRT / Citadel / Optiver. The vocabulary ("positive control," "calibration") is unusual in CS interviews and reads as scientific maturity.

**Read more.** Any experimental physics / molecular biology methods text on positive controls. The CS analog is in fault injection / chaos engineering literature, but the *intent* is different (chaos engineering tests robustness; this tests measurement resolution).

---

## §5. Diagnose mechanism — `perf stat` interpretation  `[X]`

### 5.1 The counters and what they mean

**What.** `perf stat` exposes CPU performance counters. The high-ROI set:

| Counter | What it measures | When it tells you something |
|---|---|---|
| `cycles` | Total CPU cycles (wall-clock-adjusted) | Always; basis for cycles/msg |
| `instructions` | Total instructions retired | With cycles: gives IPC |
| `branches` | Total branches taken | With branch-misses: gives mispredict rate |
| `branch-misses` | Branch mispredictions | High rate → branchy hot path → fast/slow split candidate |
| `cache-references` | L1+L2 lookups (loads + stores) | Denominator for cache-miss rate |
| `cache-misses` | LLC (last-level cache) misses | High rate → data layout problem → container-choice candidate |
| `LLC-loads` / `LLC-load-misses` | LLC loads & misses specifically | More precise than `cache-*` for understanding LLC behavior |
| `dTLB-load-misses` | Data TLB misses | High → working set spans many pages → hugepages candidate |
| `context-switches` | Voluntary + involuntary context switches | Should be ~0 under proper pinning; nonzero = noise contamination |
| `cpu-migrations` | Process moved to a different CPU | Should be 0 under pinning; nonzero = pinning failed |

**Why it matters.** Latency tells you *that* something is slow. Counters tell you *why*. The leap from "I optimized something" to "I optimized this specific mechanism" is what makes a story credible.

**How (in this project).**
- `scripts/perf_stat_lobd.sh` attaches `perf stat -p <lobd_pid>` after the READY-line handshake.
- Derived metrics in the analyzer:
  - `cycles_per_msg = cycles / applied_msgs`
  - `IPC = instructions / cycles` — typical hot path: 1.0–3.0; lower means stalls (cache, branch, data dep); higher means well-scheduled
  - `branch_miss_rate = branch_misses / branches` — typical: < 1% on hot paths; > 5% is suspicious
  - `cache_miss_rate = cache_misses / cache_references` — typical: < 5% on hot paths; > 20% means data layout problem

**Interpretation patterns.**

| Symptom | Likely cause | M8 candidate |
|---|---|---|
| Low IPC (< 1.0), high cache-miss rate | Memory-bound; pointer-chasing | Replace `std::map` with sorted vector; replace `std::deque<OrderId>` with intrusive list |
| Low IPC, high branch-miss rate | Branchy code; unpredictable control flow | Fast/slow path split; sort inputs to make branches predictable |
| High IPC (> 3), low miss rates, slow latency | Algorithm itself is slow; CPU is doing useful work but lots of it | Algorithmic improvement (e.g., O(N) → O(1) cancel) |
| Nonzero context-switches under pinning | Measurement contamination | Strengthen environmental hygiene (§2.1) |
| Nonzero cpu-migrations under pinning | Pinning isn't actually applied | Verify with `taskset -p <PID>` |

**Signal value.** Tier-A across all firms. Interviewers ask "did you look at `perf stat` output?" specifically to filter for this. Being able to *interpret* the counters (not just collect them) is what distinguishes a serious candidate.

**Read more.**
- Brendan Gregg, *Systems Performance* §6.6 (CPU performance analysis tools).
- Intel Optimization Reference Manual, §B.3 (Top-Down Microarchitecture Analysis) — overkill for M7 but worth skimming for vocabulary.
- `man perf-stat` and `man perf-list` on your host — and verify counter availability in Week 1.

---

## §6. Putting it all together

The five sections above are not independent — they're a pipeline:

1. **§1 — Reproducibility** is the gate. Without it, nothing downstream is trustworthy.
2. **§2 — Suppress controllable noise.** Most variance comes from frequency scaling, scheduling, and warmup. Eliminate before measuring.
3. **§3 — Avoid measurement artifacts.** Even with a clean environment, your generator and your instrumentation can introduce artifacts (coordinated omission, timestamp overhead). Design them away.
4. **§4 — Attribute & quantify.** Now you can decompose costs into stages and quantify the residual noise floor. This is what M7 is *for*.
5. **§5 — Diagnose mechanism.** Latency numbers + counters → "this got X% faster *because* cache-miss rate dropped Y%" — which is the M8 case-study story.

Each section's techniques are needed for the next to be meaningful. Skip §2 and §4's noise-floor numbers will be useless. Skip §3.1 and §4's p99 numbers will be lies. Skip §5 and M8 case studies devolve into "got faster" without mechanism.

**The interview punchline.** Almost no student project does §3.1, §4.3, §4.4, and §5 well. Doing them — and being able to *articulate why each one is necessary* — is the differentiator between "built a matching engine" and "thinks like a quant infra engineer." The techniques in §1 and §2 are table stakes once you've done §3–§5; you don't lead with them, but you must be able to defend them when probed.
