> # M6

# M6 interview-prep summary

Organized by theme, ranked by interview leverage. Skim what's strong, internalize the talking points, acknowledge the weaknesses before the interviewer finds them.

## 1. Determinism infrastructure — your strongest quant interview hook

Quant infra is judged first on **reproducibility**, then on speed. You built two layers of it.

**State hash (FNV-1a) in `BookSummary`** — `src/engine/book/order_book.cpp:215-262`. The engine commits the entire book — live-order count, every price level, every order's full state (id, side, price, qty, symbol) — into a single `uint64_t`. Bids and asks are hashed under different prefix bytes (`'B'`, `'A'`) so a mislabeled order can't collide with a correctly labeled one.

**Message hash (FNV-1a) in `replay_ref`** — `benchmarks/e2e/replay_ref.cpp:84-90`. Same algorithm, applied to the input stream. Lets you separately verify "did we generate the same stream?" vs "did we produce the same engine state?"

**Cross-process determinism oracle** — `scripts/compare_3proc_hash.sh`. Runs `replay_ref` (in-proc) and the 3-proc pipeline on the same `<scenario, N>`, then `grep`s `state_hash=` from each and asserts equality. Empirically passes on all 3 scenarios at N=200k.

> **Pitch (60 sec):** "Determinism is non-negotiable for quant infra, so I built two hash oracles. The book's `state_hash` is FNV-1a over the full live-order set, with side-prefix bytes to prevent bid/ask collision. The replay's `msg_hash` is FNV-1a over the input stream. Then a single bash oracle runs the engine in-process and across three IPC processes and asserts both hashes match — that's how I proved that my wire format and UDS transport don't perturb engine state. It runs in under a second on 200k messages."

**Why FNV-1a specifically:** simple, branchless, no library dependency, byte-stable across compilers. Not cryptographic — and you don't need cryptographic here.

## 2. CPU pinning architecture — the Card 002 story

You implemented **both** mechanisms deliberately:

- External: `taskset -c <core>` in the scripts.
- In-binary: `sched_setaffinity` via `affinity.hpp::pin_thread_to_cpu`.

The **sentinel default** (`--cpu-core -1` = skip pinning) means the two modes are truly disjoint — no double-pin shadow. You verified via `taskset -p $PID` that the kernel actually applies the requested mask in both modes.

> **Pitch:** "I implemented CPU pinning two ways: `taskset` externally and `sched_setaffinity` in-binary, because I wanted to verify they're equivalent in steady state. The catch was double-pinning — if both are active, the in-binary call silently widens taskset's mask. I fixed this by making `--cpu-core` opt-in with a -1 sentinel: the binary only calls `sched_setaffinity` if the flag is explicitly passed. Then I verified via `/proc/<pid>/status` that the affinity mask matches what the script claims. That's also the design that lets me run Card 001 (pin on/off) and Card 002 (taskset vs in-binary) as clean single-variable experiments."

This story has interview legs: "I considered both options, built both, planned a measurement to choose between them" is the strong-candidate pattern.

## 3. C++ zero-cost abstraction patterns

These show language fluency. Bring up unprompted if the conversation goes near "how did you avoid overhead?"

**CRTP for scenario generators** — `src/exchange/generator/scenarios.hpp:62-106`. `GeneratorBase<Derived>` calls `static_cast<Derived*>(this)->generate_impl(...)` — static dispatch, no vtable, generator body inlinable. Your inline comments explicitly note the friend-vs-concepts trade-off you made.

> **Pitch:** "Scenario generation runs in a tight loop generating millions of messages — virtual dispatch overhead would be noise but also unnecessary since the type is known at compile time. CRTP gives me polymorphism without indirection. I tried concepts first but ended up with friend + private `generate_impl` because the public interface should live on the base class only."

**Templated sink in `InProcGateway::handle`** — `src/gateway/inproc_gateway.hpp:26-54`. `template <class Sink>` instead of `std::function<void(Event)>`. Your inline comment captures the reasoning: "avoids std::function overhead (nice for measurement)."

> **Pitch:** "I deliberately kept the gateway's event sink as a template parameter rather than `std::function`. `std::function` has type-erasure indirection that would muddy in-proc latency numbers. Engine code still uses `std::function` because polymorphism matters more there than the few cycles."

**`std::variant` + `std::visit` + `if constexpr`** — `wire.hpp`, `replay_ref.cpp:113-123`, `inproc_gateway.hpp:39-46`. Closed-set sum types, exhaustively dispatched at compile time, no allocations. `if constexpr (std::is_same_v<M, ts::proto::NewOrder>)` is the modern C++ idiom; calling out that you know about it signals C++17/20 fluency.

> **Pitch:** "Messages are a `std::variant` over the seven types in the wire protocol. Dispatch uses `std::visit` with `if constexpr` — exhaustive at compile time, no virtual functions, no `dynamic_cast`. Same `ClientMsg` flows through tests, in-process gateway, and across the wire."

## 4. Wire protocol design

Small but well-thought-out. Bring up if they ask about protocol design.

**Fixed-size 16-byte header** — `wire.hpp:42`. version (u16) + msg_type (u16) + length (u32) + seq (u64). Fields ordered for natural alignment, length-prefix enables defensive framing.

**Strict framing** — `wire.hpp:274`: `if (d.length != bytes.size()) return std::nullopt;` Rejects mismatched payload sizes. Defensive against over/under-read.

**Explicit endianness** — `htole*` / `le*toh` macros even on loopback. Not premature; it's correctness-as-protocol-convention. The whole codec uses `memcpy` (no UB from misaligned reads) and is endianness-explicit.

**Stack-allocated frame** — `wire.hpp:60-66`: `std::array<std::byte, kMaxFrameSize=256>` inside `Frame`. No heap allocation in the recv/send loop. `bytes_view()` returns a `std::span<const std::byte>` — non-owning view.

> **Pitch:** "Even though I'm only on UDS loopback today, I made the wire format explicitly little-endian via `htole*`. That's the discipline I'd need crossing a real network. The frame is a fixed 256-byte `std::array` on the stack — no heap allocation on the hot path. And the codec uses `memcpy` instead of pointer casts to avoid undefined behavior on misaligned access."

## 5. Engine / order-book design

**Price-level container choice** — `order_book.hpp:59-60`:

```other
std::map<Price, deque<OrderId>, std::greater<Price>> bids_;
std::map<Price, deque<OrderId>> asks_;
```

The custom comparator on bids means `bids_.begin()` is *always* the best bid (highest), symmetric to asks' default (lowest). Both sides have O(log K) insert and O(1) best-price lookup. Deque per level gives FIFO.

**O(1) cancel lookup** — `std::unordered_map<OrderId, LiveOrder> live_` is the source of truth; `cancel` reads side+price from there, then erases from the level deque.

**Deterministic fill ordering** — `order_book.cpp:165-166`: taker fill emitted first, maker second. Comment explicitly calls it deterministic.

> **Pitch:** "Bids and asks both use `std::map`, with bids using `std::greater` as the comparator. That way `begin()` gives me the best price on both sides — symmetric code, no special-casing. Cancel is O(1) lookup via `unordered_map` keyed by order id, then O(N-at-price) erase from the deque. I know the deque erase is the obvious next M8 optimization — replace it with an intrusive list and store the iterator in `LiveOrder` for O(1) cancel."

That last sentence is the **acknowledge-weakness-first** move. Do it.

## 6. Process orchestration discipline

Small but shows you've shipped before.

- **READY-line handshake**: each process prints `READY ...` once bound. Runner polls log via `grep` (50ms resolution, portable, no inotify dependency).
- **Launch order**: lobd → gateway → exchange. Receivers bind before senders dial — prevents the "packets into not-yet-bound socket" bug you actually hit and fixed earlier.
- **SIGTERM → wait → SIGKILL** escalation in `stop_pid`.
- **`trap cleanup EXIT`** guarantees socket files and orphan procs are reaped on any exit path.
- **`${PIPESTATUS[0]}`** to capture the *generator's* exit code through a `tee`.
- **Two-phase argument parser** (Phase 1: consume flags out of `$@`; Phase 2: assign positionals) — fixed a real bug where `--cpu-core` slid into the SCENARIO positional slot.

> **Pitch (if asked about ops/scripts):** "The shell runner uses a READY-line handshake — each process prints a known marker after binding, the runner polls. Then SIGTERM-with-fallback-to-SIGKILL, an EXIT trap for socket cleanup, and PIPESTATUS to capture the generator's status through the pipeline. None of these are individually clever, but together they catch the actual failure modes — silent drops on un-bound sockets, leaked sockets on Ctrl-C."

## 7. Measurement primitives

Three small but correct choices:

- **`CLOCK_MONOTONIC_RAW`** in `clock.cpp:8` — not affected by NTP slew or wall-clock jumps. The correct clock for latency measurement; `CLOCK_REALTIME` would be wrong.
- **`std::nth_element`** in `percentiles.hpp:18` — O(n) partial sort to extract a single percentile. Critical when you're computing percentiles over millions of samples per run.
- **Per-message timestamp pairs** in `InProcGateway::GwMetrics` — `total_ns` and `lob_ns` captured separately, so engine-only latency is attributable.

## 8. Weaknesses to acknowledge first (before the interviewer does)

Cleanest interview move: **name your own next M8 case studies before the interviewer asks "what would you improve?"**

| **Today**                                                  | **Next case study**                                                   |
| ---------------------------------------------------------- | --------------------------------------------------------------------- |
| `std::function` EventSink in engine path                   | Template-parameterize sink end-to-end (compare via inproc_runner)     |
| `std::deque<OrderId>` per level + `std::find` for cancel   | Intrusive list with iterator stored in `LiveOrder` → O(1) cancel      |
| `std::map` for price levels (node-based, cache-unfriendly) | Sorted vector / B-tree of (price, level) → better cache locality      |
| Allocator: default `new/delete` everywhere                 | Pool / PMR allocator for `LiveOrder` and `Level` nodes                |
| No NUMA awareness                                          | First-touch policy on the pinned core (already half-done via pinning) |

> **Pitch:** "The engine works correctly but I know exactly what's slow about it: cancel is O(level size) because I `std::find` into the deque, and the price-level map is node-based which is cache-unfriendly. Those are my top two M8 case studies — replace the deque with an intrusive list so cancel becomes O(1), and replace the map with a sorted vector. Each will be a measured before/after card with `perf stat` counters."

## 9. Things to NOT oversell

- No SPSC ring buffer yet — only UDS dgram. Don't claim "low-latency IPC"; claim "correct, message-oriented IPC with deterministic framing."
- No lock-free anything. The pipeline is single-threaded per process.
- No kernel bypass, no DPDK, no shared memory. Future work.
- The wire format is "version 1, fixed-size header, length-prefixed" — *not* "FIX" or "ITCH-style". Don't claim parity with industry protocols.

## 10. One-paragraph pitch for the project as a whole

> "It's a 3-process trading pipeline — exchange simulator, gateway, matching engine — connected over UDS datagram with a versioned, length-prefixed wire format. The engine is IO-free so the same library runs in microbenchmarks, an in-process pipeline, and the 3-proc setup; the difference between those modes is how I attribute latency. Determinism is verified by an FNV-1a hash of the full book state, compared across the in-proc and 3-proc paths. CPU pinning is implemented two ways with a single-variable comparison planned. The goal isn't to be the fastest matching engine — it's to be a reproducible measurement platform where I can do controlled C++ performance case studies, each with a hypothesis, a workload, and `perf stat` counters tied to the predicted mechanism."

That's a credible, hireable summary. Use it as the first answer to "what is this project?"

## What's not on the branch yet (be honest if asked)

- Cards 001 and 002 not written yet (planned, infrastructure ready)
- M7 measurement suite — per-message timestamp logs, percentile aggregator, `null_engine` mode, `perf_stat_lobd.sh` — not built yet
- 4-scenario matrix incomplete (`aggressive_cross`, `burst` missing)
- M8 case-study cards — none filled in

Frame these as "what I'm doing next month" if asked. They're already in your todo list and have a clear plan, which is the right interview answer.

## Bottom line

The branch has more interview-worthy substance than a typical student trading project, because of two things: **the determinism oracle** (cross-process state-hash comparison is unusual and very quant-shaped) and **the deliberate two-mechanism CPU pinning** (designed for a measurement comparison, not just because pinning is "good"). Lead with those two stories. The engine and wire format are solid and credible but not unusual. Acknowledge the O(N) cancel and node-based map as your top M8 candidates and you'll come across as someone who knows what they built and what's next — which is the actual signal interviewers are looking for.

