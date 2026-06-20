# M7 interview prep — daily study deck

Open this 15 min/day during M7. Re-read before any phone screen. Numbers are placeholders until M7 lands real measurements; pitch structure is final.

Companion to [`M7-plan-v2.md`](M7-plan-v2.md) (operational checklist) and [`M7-techniques.md`](M7-techniques.md) (conceptual reference for the techniques behind §1 and §4).

---

## 1. The 30-second elevator pitch (memorize)

> *"It's a 3-process C++ trading pipeline — exchange simulator, gateway, matching engine — connected over UDS datagram with a versioned, length-prefixed wire format. The engine is I/O-free so the same library runs in microbenchmarks, an in-process pipeline, and the 3-process setup; the difference between those modes is how I attribute latency.*
>
> *Before optimizing, I built a measurement-validity layer. lobd has stage-attribution modes — null and full match — so I can decompose wire-to-wire latency into transport, codec, and engine cost. I separate throughput-mode runs from attribution-mode runs because sampled timestamps perturb throughput. I quantified the noise floor on my pinned-core setup and set a threshold: I won't claim an optimization unless the delta exceeds it. As a positive control I inject a known artificial delay and verify the harness can resolve it.*
>
> *Correctness is gated by a deterministic FNV-1a state hash compared between the in-process reference and the 3-process pipeline. If that fails, I refuse to compare perf numbers."*

Rehearse aloud daily. If you can't say it in 30 seconds without padding, the doc has waste.

---

## 2. Vocabulary translation (PhD → industry)

Use the right column in resume, README, interview opener. Keep the left column for `001_measurement_validity.md` (where the rigor is the point).

| Your PhD term | Industry term that lands |
|---|---|
| "null/decode/match attribution modes" | **"wire-to-wire latency decomposition"** / "stage timestamps" |
| "synthetic perturbation calibration" | **"sanity check — if I inject a known 1 µs delay, does the harness catch it?"** |
| "minimum detectable effect" | **"noise floor"** |
| "measurement-validity infrastructure" | **"I don't trust a benchmark until I can reproduce it three times"** |
| "FNV-1a state hash oracle" | **"deterministic replay verified by bit-exact state hash"** |
| "open-loop generation" | **"open-loop, no coordinated omission"** |
| "single-variable experiment" | **"before/after with one knob changed"** |
| "coefficient of variation" | "run-to-run noise" (CV is fine in writing, "noise" in speech) |

---

## 3. Resume bullet drafts (iterate as M7/M8 ship)

### After M6 (now)

> 3-process C++ matching pipeline (exchange → gateway → lobd) over UDS datagram with a versioned wire format and deterministic replay verified by FNV-1a state-hash equality between in-process reference and 3-process pipeline. Per-process CPU pinning via `taskset` and `sched_setaffinity`.

### After M7 (target end-of-May to early-June 2026)

> Added wire-to-wire latency decomposition across pinned cores; quantified host noise floor via repeated runs and a synthetic-perturbation positive control. Reports p50/p99/p99.9 stage latencies, throughput, and `perf stat` counters per scenario. All optimization claims gated by effect-size > noise floor.

### After each M8 case study (fill placeholders with real numbers)

> Reduced cancel-path p99 from **X ns → Y ns** via O(1) intrusive-list cancel; effect verified above noise floor across 10 pinned repeats. Cache-miss rate dropped from **A% → B%** per `perf stat`. Effect appears in `cancel_heavy` scenario but not in `add_only`, confirming mechanism.

---

## 4. Q&A pairs (rehearse the shape, fill your numbers)

### Q: What's your p99 latency?

> Wire-to-wire p99 on `cross_mixed` at N=1M is **X ns**. Stage breakdown: exchange→gateway transport **Y ns**, gateway processing **Z ns**, lob decode **A ns**, lob apply **B ns**. The hot stage is lob apply, dominated by [order-book lookup / level erase / etc.], which is my next M8 case study.

### Q: How do you know that's not noise?

> Ran the same config 10× pinned with governor=performance and Turbo off. My CV is **X%** for throughput and **Y%** for p99. I set a noise floor at 3× MAD; any optimization claim has to exceed that. I also verified sensitivity with a positive control — inject a known **K µs** delay, confirm the harness resolves it.

### Q: What's coordinated omission?

> It's when the generator slows down because the consumer is slow, so worst-case latencies never get recorded — they get coordinated-out of the histogram. Gil Tene's talk is canonical. My exchange generates open-loop, paced by an internal counter, not by gateway acks. So if lobd back-pressures, I see drops / `seq_errors`, not artificially-good p99.

### Q: Why `std::map` for price levels? Isn't that cache-unfriendly?

> Yes — it's the obvious next case study. `std::map` is node-based; each level access can L1-miss. The replacement is a sorted vector of `(price, level)` or an intrusive B-tree. Haven't done it yet because I want measured before/after with perf-counter attribution. Cache-miss rate should drop, and the effect should show up in `add_only` and `cross_mixed` but not `cancel_heavy` — that mechanism check is part of the experiment design.

### Q: Why `std::function` in some places but not others?

> Template-parameterized the in-proc gateway's event sink because it's on the measured path and `std::function` has type-erasure indirection. Engine still uses `std::function` because polymorphism matters more there, and engine cost is dominated by order-book ops, not callback dispatch. I'd revisit if microbench showed callback overhead > 5% of engine cycles/msg.

### Q: Tell me about your data structures.

> Bids and asks both `std::map`, with bids using `std::greater` so `begin()` gives best price on both sides — symmetric code, no special-casing. Per-level FIFO is `std::deque`. Cancel is O(1) lookup via `unordered_map` keyed by order id, then O(N-at-price) erase from the deque. I know the deque erase is the next M8 win — replace with an intrusive list and store the iterator in `LiveOrder`.

### Q: What's your next optimization?

> O(1) cancel. Currently `std::find` over the deque at the price level — O(N-at-price). Replace with intrusive list, store iterator in `LiveOrder`. Expect it to land biggest on `cancel_heavy`, nothing on `add_only` — that scenario-mechanism alignment is how I separate "real fix" from "lucky variance".

### Q: Did you actually measure, or are you guessing?

This is the trap question. **Always answer with a measurement or admit you haven't.**

> On [X] I measured — here are the numbers. On [Y] I have a hypothesis and the experiment plan but haven't run it yet.

Never claim a number you didn't measure.

### Q: What would you do with a real production-tuned machine?

> One-paragraph answer: IRQ affinity off the measurement core, `nohz_full`, 1 GB hugepages, HT sibling disabled, NUMA-local allocation, `isolcpus=` on the cmdline. I know the ladder; I've applied governor + Turbo + paranoid + service-stop on my dev host, and I've documented what I haven't.

---

## 5. Firm-tier guidance — what to lead with

| Firm | Lead with | Why |
|---|---|---|
| **Jane Street** | Determinism + reproducibility + statistical rigor (noise floor, MDE) | S&T podcast emphasizes these; OCaml culture values bit-exact replay |
| **Two Sigma** | Statistical rigor + measurement methodology | Strong stats culture; A/B-style framing lands |
| **Citadel / Citadel Securities** | Wire-to-wire decomposition + concrete latency numbers + perf-counter mechanism | They want numbers + mechanism, not just "I optimized" |
| **HRT** | Per-stage attribution + `perf stat` fluency + acknowledged weaknesses | They like candidates who know what they didn't optimize and why |
| **Jump Trading** | Low-level systems (cache, branches, NUMA) + reproducibility | Hardware-fluent culture |
| **Optiver** | Carl Cook "When a microsecond is an eternity" mental model — every cycle matters | Discipline over cleverness |
| **DRW / IMC / Tower / Akuna** | Same playbook as Citadel-style: numbers, mechanism, controls | Mid-tier HFT, same expectations |

---

## 6. What NOT to oversell

When asked about any of these, acknowledge directly — don't dodge.

- This is **not a production trading platform**.
- UDS datagram is **not exchange networking**.
- Scenarios are **controlled stressors**, not historical market truth.
- `null/decode/match` subtraction is an **attribution heuristic**, not formal causal proof.
- Wire protocol is **version 1, fixed-size header, length-prefixed** — *not* FIX or ITCH.
- **No lock-free, no SPSC, no kernel bypass.** Future work.

Template answer: *"I haven't done [X] — it's possible but the latency contribution at my current scale is dominated by other stages first. I'd profile to decide when it's worth it."*

---

## 7. Reading list (1–2 items/week during M7)

**Mandatory (read in Week 1):**
- Gil Tene, *How NOT to Measure Latency* (Strange Loop 2015) — coordinated omission
- Carl Cook, *When a Microsecond Is an Eternity* (CppCon 2017) — Optiver low-latency mindset

**High value:**
- David Gross, *Trading at Light Speed* (Meeting C++ 2022)
- Erik Rigtorp's blog (rigtorp.se) — esp. SPSC queue, false sharing posts
- Charles Frasch (HRT), *Single Producer Single Consumer Lock-free FIFO from the Ground Up* (CppCon 2023)
- Jane Street **Signals & Threads** podcast — esp. episodes on performance and OCaml-to-FPGA

**Background / browse:**
- Hudson River Trading "Life at HRT" engineering posts
- Citadel Securities engineering blog (market-data-stack posts)
- Optiver Insights blog

---

## 8. Daily / weekly cadence

- **Daily (15 min):** open this doc → re-read elevator pitch → pick one Q&A → rehearse aloud.
- **Weekly:** read one item from §7. Add any unfamiliar vocabulary to §2.
- **End of each M7 week:** update §3 resume bullets with what shipped that week.
- **Before any phone screen:** read §1, §4, §5 (the row for that firm), and §6.

---

## 9. Things to add as M7 / M8 progress

- Replace **X / Y / Z / A / B / K** placeholders in §1, §3, §4 with real numbers as runs land.
- After each M8 case study: write a 60-second case-study pitch (hypothesis → workload → metric → result → mechanism) and add to §4.
- Any vocabulary an interviewer uses that you didn't recognize: add to §2 immediately.
- Any question you got asked that's not in §4: add it the same day.
