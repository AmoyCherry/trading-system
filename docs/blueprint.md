The trading system consists of a LOB, gateway and exchange simulator, is just a platform to conduct performance case study.

The high ROI version is:
- a small, correct core +
- a disciplined measurement loop +
- a few high-signal optimization case studies you can explain.


### Guiding rules (keeps ROI high)
1. Correctness before speed
    - Invariants + deterministic replay are your safety net.
2. One baseline first
    - Tag v0-baseline and never “improve” it without measuring.
3. Every optimization is an experiment
    - Hypothesis + workload + metric + result + interpretation.
4. Don’t optimize what you can’t measure
    - Microbench + E2E latency distributions come early, not late.
5. Minimal features
    - New/Cancel + price-time priority is enough to be impressive.

Use `spec → pseudocode → tests → implementation` for places where misunderstandings create expensive rewrites or subtle bugs.
- Matching rules (price-time, partial fills, crossing behavior)
- Cancel semantics
- Order lifecycle (accepted/rejected, duplicate IDs, unknown cancels)
- Deterministic replay logic
- Wire/serialization rules (when you harden protocol later)
- Any “hot path refactor” that could change correctness

For these, the cycle is extremely high ROI:
- Spec: 10–30 bullet rules (short)
- Pseudocode: for 1–3 critical functions (matching loop, cancel)
- Tests: golden scenarios + invariants
- Implementation: simplest passing version

## Case Study Card
For every change (case study), write a 10–15 line “experiment card”:
- Hypothesis: what will improve and why (cache, branch, alloc, contention)
- Change: what exactly you changed
- Workload: input distribution (order sizes, cancels %, price distribution)
- Metrics: throughput + p99 + 1–2 perf counters
- Result: numbers before/after
- Interpretation: what counter moved and why it matches (or contradicts) the hypothesis
- Decision: keep / revert / follow-up

## Milestones
> [plan](https://chatgpt.com/s/t_69876b75d9c881918f4d243f0db67281)
> [important signals](https://chatgpt.com/s/t_69b0cb6a57f0819190d334318bb03a22)

- [M0 — Skeleton + toolchain (3–5 days)](https://chatgpt.com/s/t_69877fb39098819181b1971b92d7dc47)
- [M1 — Spec + message model + correctness harness scaffolding (1–1.5 weeks)](https://chatgpt.com/s/t_69878034bb708191aa67705904dd2936)
- [M3 — Microbench baseline + perf hooks (1–2 weeks)](https://chatgpt.com/s/t_6997ca91d8848191ae4f7dd91fdf6581)
- [M4 — In-proc pipeline integration (1 week)](https://chatgpt.com/s/t_699a63ce0a148191a99d4f8ed44278b9)
- [M5 — Formalize protocol + add transport backends (2 weeks)](https://chatgpt.com/s/t_699d17ddeaf88191852f19a49b0c08ac)


```text
trading-system/
├── CMakeLists.txt
├── CMakePresets.json                 # optional, but convenient
├── cmake/
│   ├── ProjectOptions.cmake          # warnings/sanitizers/options
│   ├── Sanitizers.cmake
│   └── Warnings.cmake
├── docs/
│   ├── milestones.md
│   ├── architecture.md
│   ├── spec.md
│   └── experiments/
│       └── 000_template.md
├── src/
│   ├── CMakeLists.txt
│   ├── common/
│   │   ├── CMakeLists.txt
│   │   ├── protocol/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── messages.hpp          # NewOrder/Cancel/Ack/Fill/Reject (message model)
│   │   │   └── wire.hpp              # (later) pack/unpack + header/version/seq
│   │   ├── time/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── clock.hpp
│   │   │   └── clock.cpp
│   │   ├── stats/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── percentiles.hpp
│   │   │   └── percentiles.cpp
│   │   └── util/
│   │       ├── CMakeLists.txt
│   │       ├── affinity.hpp
│   │       ├── affinity.cpp
│   │       ├── cli.hpp
│   │       └── cli.cpp
│   ├── engine/
│   │   ├── CMakeLists.txt
│   │   ├── engine.hpp
│   │   ├── engine.cpp
│   │   └── book/
│   │       ├── CMakeLists.txt
│   │       ├── order.hpp
│   │       ├── order_book.hpp
│   │       └── order_book.cpp
│   ├── lobd/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── transport/
│   │       ├── CMakeLists.txt
│   │       ├── transport.hpp         # abstract send/recv interface
│   │       └── uds_transport.cpp      # (later) UNIX domain socket transport
│   ├── gateway/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── transport/
│   │       ├── CMakeLists.txt
│   │       ├── transport.hpp
│   │       └── uds_transport.cpp
│   └── exchange/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── generator/
│           ├── CMakeLists.txt
│           ├── scenarios.hpp          # scenario config structs
│           └── scenarios.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_order_book.cpp
│   │   └── test_engine_basic.cpp
│   ├── replay/
│   │   ├── test_replay_hash.cpp
│   │   └── traces/
│   │       └── tiny_trace.json        # deterministic test trace
│   └── property/
│       └── test_invariants.cpp
├── benchmarks/
│   ├── CMakeLists.txt
│   ├── micro/
│   │   └── bm_matching.cpp
│   └── e2e/
│       ├── CMakeLists.txt
│       ├── runner/
│       │   └── main.cpp               # launches 3 procs + collects results
│       ├── scenarios/
│       │   ├── add_only.json
│       │   ├── cancel_heavy.json
│       │   ├── aggressive_cross.json
│       │   └── burst.json
│       └── analysis/
│           └── summarize.py           # percentiles + summary table
├── scripts/
│   ├── build.sh
│   ├── run_micro.sh
│   ├── run_e2e.sh
│   └── perf_stat_lobd.sh
├── .clang-format
├── .clang-tidy
├── .gitignore
└── README.md

```