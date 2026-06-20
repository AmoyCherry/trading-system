# Trading System Development Strategy

> Below is a milestone doc blueprint you can drop into `docs/milestones.md` and use as your "project constitution" through August.
>
> After that, I'll address your question about why I suggested protocol/common earlier vs later, and what a practical compromise looks like (so you keep your "shared objects first" mindset without prematurely locking yourself into a bad interface).

---

# Minimum Trading System (3-process) — Milestone Blueprint (Feb → Aug 2026)

## 0) Purpose

Build a minimal but realistic trading pipeline:

```
exchange_sim → gateway → lobd → gateway → exchange_sim
```

…and use it to produce **benchmark-driven, low-level performance case studies** in C++.

### Success criteria by late Aug 2026 (application-ready)

You can point an interviewer to a repo that has:

**Correctness**
- Deterministic replay: same input trace ⇒ same output hash
- Invariants/property tests for the order book + matching logic
- Golden scenario tests (handcrafted)

**Performance**
- Microbench suite (Google Benchmark) for the matching engine hot paths
- E2E latency suite with percentiles (p50/p90/p99/p99.9) + throughput
- A "null engine" E2E control mode to isolate transport vs engine contribution
- `perf stat` scripts and at least 3 case studies with counters + explanations

**Engineering polish**
- One-command build, one-command run for microbench and E2E
- Reproducible configs + scripts for CPU pinning and result export
- Clear docs: architecture diagram + "how to reproduce results"

---

## 1) Non-goals (explicitly out of scope)

These are common time sinks; only touch them if you have a specific experiment.

- FIX protocol compliance
- Persistence / recovery / journaling
- Many order types (IOC/FOK/peg/stop/etc.)
- Multi-symbol matching complexity (start with 1 symbol or small fixed set)
- Kernel bypass (DPDK/AF_XDP), NUMA tuning, huge pages (optional later only if clearly measurable)
- "Production-grade" risk checks (keep gateway minimal)

---

## 2) Repository structure (target layout)

```
trading-system/
  docs/
    milestones.md
    architecture.md
    experiments/
      000_template.md
      001_allocator.md
      ...
  src/
    common/
      protocol/       # message structs + serialization (formalized later)
      time/           # timestamp utilities
      stats/          # histograms/percentiles + CSV writer
      util/           # pinning, args, error helpers
    engine/           # pure matching engine library (no IO)
      book/
      engine.hpp/.cpp
    lobd/             # process wrapper around engine
      main.cpp
      transport/
    gateway/
      main.cpp
      transport/
    exchange/
      main.cpp
      generator/
  tests/
    unit/
    property/
    replay/
  benchmarks/
    micro/
    e2e/
      scenarios/
      runner/
      analysis/
  scripts/
    build.sh
    run_micro.sh
    run_e2e.sh
    perf_stat_lobd.sh
  artifacts/          # gitignored: results, logs, plots
```

---

## 3) Design principles (hard rules)

1. **Baseline first, then experiments.**
   - Tag a baseline release (`v0-baseline`) once correctness + microbench exist.
2. **No optimization without a measurement plan.**
   - Every change must say: hypothesis → metric → workload.
3. **No performance work without correctness guardrails.**
   - Tests + determinism must pass before comparing performance numbers.
4. **Single-variable experiments.**
   - One technique at a time. No "bundle of tweaks."

---

## 4) Milestones

### M0 — Skeleton + toolchain (3–5 days)

**Goal:** You can build/run all targets (even stubbed) and have CI sanity.

**Deliverables**
- CMake project (or Meson/Bazel if you prefer; CMake is simplest)
- Targets compile:
  - `engine` library (stub ok)
  - `lobd`, `gateway`, `exchange_sim` executables (stub ok)
  - `unit_tests`
  - `microbench` (empty benchmark ok)
- Formatting + linting config (clang-format, optional clang-tidy)
- Scripts:
  - `scripts/build.sh` (single entry)
  - `scripts/run_micro.sh` and `scripts/run_e2e.sh` (placeholder)

**Exit criteria**
- Fresh clone → build works on your machine in one command.

---

### M1 — Spec + message model + correctness harness scaffolding (1–1.5 weeks)

**Goal:** You know what "correct" means before writing fast code.

**Deliverables**
- `docs/architecture.md` (simple diagram + data flow)
- `docs/spec.md` (or in architecture doc):
  - order lifecycle
  - matching rules (price-time)
  - reject reasons (minimal)
- Minimal **command/event structs** (in C++ headers)
  - `NewOrder`, `Cancel`, `Ack`, `Fill`, `Reject`
- Test scaffolding:
  - unit test framework wired (Catch2/GoogleTest—either is fine)
  - `tests/replay/` skeleton: run a trace → produce output → hash

**Exit criteria**
- At least 5 "golden" tests described (even if not all implemented yet)
- Basic determinism harness compiles

> Note: This "message model" is **not yet "the protocol"** (details in section 6).

---

### M2 — Matching engine baseline (single-process library) (2–3 weeks)

**Goal:** A correct matching engine core that can be called from tests/benchmarks.

**Scope**
- `New + Cancel`
- price-time priority
- partial fills
- simple symbol handling (1 symbol is fine)

**Deliverables**
- `src/engine/` provides:
  - `Engine::on(NewOrder)`
  - `Engine::on(Cancel)`
  - output events via callback or output buffer
- Correctness tests:
  - add-only builds book state correctly
  - cross/match produces expected fills
  - cancel removes the right order
  - FIFO at same price level

**Exit criteria**
- "Golden scenarios" pass
- Deterministic replay for fixed seed inputs passes

---

### M3 — Microbench baseline + perf hooks (1–2 weeks)

**Goal:** A stable baseline measurement of the engine hot path.

**Deliverables**
- Google Benchmark targets:
  - `BM_AddOrder`
  - `BM_CancelOrder`
  - `BM_MatchAggressive`
- Workload generators for microbench:
  - fixed seeds
  - pre-generated order streams to avoid allocations inside timing loop
- `scripts/run_micro.sh` produces:
  - benchmark output (JSON/console)
  - machine info header (CPU model, kernel, compiler flags)

**Exit criteria**
- You can run microbench 5 times and results are "stable enough" (no wild swings)
- Tag `v0-baseline` in git

---

### M4 — In-proc pipeline integration (1 week)

**Goal:** You can simulate exchange→gateway→engine without IPC noise.

**Deliverables**
- `exchange_sim` calls gateway handler in-process
- gateway forwards to engine in-process
- full round-trip messaging works
- instrumentation (timestamps) works in-process

**Exit criteria**
- E2E "in-proc" p50/p99 exists (even if rough)
- Outputs match deterministic replay expectations

---

### M5 — Formalize protocol + add transport backends (2 weeks)

**Goal:** Turn message model into a *stable* protocol and support multi-process.

**Deliverables**
- Move message structs to `src/common/protocol/`
- Choose representation:
  - integer price ticks (recommended) vs double
  - qty as integer
  - fixed-size header: version + msg_type + length + seq
- Serialization:
  - "pack/unpack" functions (even if trivial memcpy for fixed-size)
  - explicit endianness strategy (even if "host order for now, loopback only")
- Transport backend #1:
  - UNIX domain sockets (good for local multi-process) **or** UDP loopback
- Optional Transport backend #2:
  - shared memory SPSC (later experiment)

**Exit criteria**
- You can run the pipeline with real serialization between components *in-process or across threads*
- Protocol has version field (future-proofing, low effort)

---

### M6 — Split into 3 processes + E2E runner (2–3 weeks)

**Goal:** Realistic 3-process architecture with reproducible E2E runs.

**Deliverables**
- Executables:
  - `lobd` (wraps engine)
  - `gateway`
  - `exchange_sim`
- Startup/ready handshake:
  - runner waits until each process is "ready" before starting measurement
- CPU pinning options:
  - `--cpu-core` flag per process (or via script)
- Basic E2E runner:
  - launches processes
  - runs one scenario
  - collects output logs

**Exit criteria**
- One command runs a full round-trip scenario successfully
- Results and logs land in `artifacts/results/<timestamp>/`

---

### M7 — E2E benchmark suite that's sensitive (2–3 weeks)

**Goal:** E2E tests that reliably reflect engine changes.

**Deliverables**
- **Two E2E modes**
  1. `null_engine`: lobd ACKs immediately (transport baseline)
  2. `match_engine`: real engine
- Scenario suite (start with 4):
  - `add_only`
  - `cancel_heavy`
  - `aggressive_cross`
  - `burst`
- Timestamp points recorded (minimal but attributive):
  - exchange send / exchange recv
  - gateway recv/send
  - lob recv / lob done
- Percentiles computed:
  - p50/p90/p99/p99.9, max, throughput
- Scripts:
  - `scripts/run_e2e.sh --scenario X --mode null|match --repeat N`

**Exit criteria**
- You can show: (match RTT − null RTT) changes when you modify engine internals
- Stage breakdown makes sense (no "mystery latency")

---

### M8 — Performance case studies (target 3–5 total) (4–6 weeks total)

**Goal:** Produce high-signal, defensible optimization stories.

**Process for each case study**
- Create `docs/experiments/00X_<name>.md`
- Record:
  - hypothesis
  - what changed
  - workloads (micro + which E2E scenarios)
  - metrics (cycles/op, p99, cache misses, branch misses)
  - result + interpretation
  - keep/revert decision

**Recommended case studies (high ROI)**
1. Allocator strategy (pool/pmr) vs new/delete
2. Data layout / container choice for price levels
3. Cancel path O(1) removal (store handles/iterators/index)
4. Fast/slow path split for common orders
5. (Optional) IPC backend comparison (UDS vs SPSC), **only if attribution is clean**

**Exit criteria**
- 3–5 experiment docs with before/after numbers + perf counters
- Baseline tag and each experiment reproducible with scripts

---

### M9 — Application polish pack (2–3 weeks in Aug)

**Goal:** Make it "interviewer-ready" and easy to trust.

**Deliverables**
- README:
  - architecture diagram
  - how to run microbench + e2e
  - "results snapshot" table
- One command demo:
  - `./scripts/demo.sh` runs a small scenario and prints p50/p99 + throughput
- Clean build instructions + dependencies
- Final resume bullets drafted from measured deltas

**Exit criteria**
- A stranger could run it from README and see meaningful results in < 10 minutes

---

## 5) Templates (copy/paste)

### Experiment card template (`docs/experiments/000_template.md`)

- **Title**
- **Baseline tag/commit**
- **Hypothesis**
- **Change**
- **Workload**
  - microbench: …
  - e2e scenarios: …
- **Metrics**
  - cycles/op:
  - p99:
  - branch-misses:
  - cache-misses:
- **Results (before/after)**
- **Interpretation**
- **Decision**
- **Follow-ups**

### Definition of Done for any "perf PR"

- [x] unit + replay tests pass
- [x] microbench run captured (JSON)
- [x] e2e run captured (CSV)
- [x] perf stat captured for lobd
- [x] experiment card updated

---

## 6) Your question: "Why not build protocol/common first? Isn't it common to do shared objects first?"

Your instinct ("shared objects first") is **common in teams** when:
- the protocol/interface is externally constrained (e.g., FIX, an exchange spec, a company-wide schema)
- many components are worked on in parallel by different people
- you want to lock an API early to coordinate work

In *your* project, you control everything and you're optimizing for **learning + measurable results**. That changes the economics:

### Why I suggested delaying "formal protocol work"

Because "protocol" often expands into:
- serialization formats
- versioning
- alignment/padding rules
- endianness decisions
- forward compatibility
- transport abstractions

If you do all that before your engine semantics are stable, you risk:
- wasting time on fields you later remove
- locking into representations you later regret (e.g., double price vs integer ticks)
- spending effort where you can't yet measure payoff

### The practical compromise (best of both)

**Do "shared objects" early, but keep them as a message model first.**
Then **promote** them into "the protocol" only when you go multi-process.

Concretely:

**Early (M1–M3):**
- Define `NewOrder/Cancel/Ack/Fill/Reject` as simple C++ POD structs.
- Use them everywhere (tests, engine API, microbench).
- Don't obsess about binary layout yet.

**Later (M5):**
- Move those structs to `common/protocol/`.
- Freeze layout, add version/seq, define serialization explicitly.
- Now it's worth the ceremony, because you're about to cross process boundaries.

This is actually very common in high-ROI development:
- **domain model first**
- **interface/protocol hardening second**
- **transport last**

You still get your "global shared objects," but you avoid premature "distributed systems paperwork."

---

## 7) What you should do immediately next (actionable)

If you want the fastest start this week:

1. Create `docs/spec.md` with 10–20 bullet rules (matching, cancel, rejects, invariants).
2. Implement message structs (`NewOrder`, `Cancel`, …) **as the message model**.
3. Write 5 golden tests.
4. Implement the simplest matching engine that passes.
5. Add microbench early; tag `v0-baseline`.

That path gets you out of "design mode" and into a tight loop where you learn fast.
