## sys design Q&A

### How do you confirm the correctness of the baseline?

### How do you protect invariants?

### How to understand bm results?
- Mean: The arithmetic average.
- Median: 50% of runs were faster than this.
- Stddev: Standard Deviation: measures how much your results vary.
- CV (%): Coefficient of Variation (stddev/mean). Jitter Metric. For low-latency code, you want this under 1%.

### What metrics do you care about?
Golden metrics at application level:
- Orders processed per second.

Golden metrics at Machine level:
- Instructions per Cycle (IPC).
- Cache-references / Cache-misses.
- Branches / Branch-misses.

#### fu - How do you improve IPC?

#### fu - How do you improve cache-misses?

#### fu - How do you improve branch-misses?

### In e2e, how do you make a non-trivial stream scenario for testing?


## Wire QA

### Why `htole32` handles unsigned types?
Endianness macros like `htole32` and `be16toh` expect unsigned types. It's implemented by bitwise shits and masks or compiler intrinsic byte-swap instructions (bswap). 

Bitwise ops on Signed Integers are risky. _TODO! Shifting negative signed integers_

### How do you convert between `std::byte` and `std:int_` during codec?
Use `std::memcpy`
1. Unaligned Memory Access (Hardware level)
   When reading packets from a network byte stream, data bytes like these integers often packed tightly. This means an integer's start address may not be a multiple of 4/8.
   - On some architectures like ARM, dereferencing an unaligned pointer causes bus errors;
2. Strict Aliasing Violations (Compiler level):
   C++ has a rule called **strict aliasing** which dictates that you cannot access an object of one type through a pointer of an incompatible type (with `char*` and `std::byte*` being the exceptions). 
   Casting a `std::byte*` to a `std::uint32_t` and deref it is a UB.

But unaligned loads can significantly increase crossing cache line accesses (not the unaligned obj itself crosses cache lines, but the following aligned objs may be affected). Which cannot be sovled by `std::memcpy`. Crossing cache lines can incur performance penalty by:
- Double Cache Misses.

The penalty is even more severe Crossing a 4KB virtual memory page.

So, the gateway or network interface card (NIC) reads the packed wire format and should immediately parse it into an internal, highly aligned data structure.

And we can use compiler intrinsics (like `__builtin_prefetch`) to fetch the next cache line into the L1 cache before the CPU actually needs it, when we know we are continousely handle byte stream. 

So we should not use `out = *reinterpret_cast<const std::int32_t*>(p);`

## Exchange

### How do you construct scenarios? And why?

The data in real world has specific distribution patterns, and neither totally random and concentrated near one single level.

todo! long tail in real world
> [order generation](https://chatgpt.com/s/t_69bb58da399c8191990eda0aee55dee6)

For case studies, the generated orders should have:
1. heavy mass near the top of book,
2. some depth further out,
3. and explicit aggressive orders that produce distinct matching regimes including:
   1. not match,
   2. match exactly one level,
   3. or sweep exactly k levels.

Why it's important that generated orders can replay?

#### make_cross
**Phase A: seed a realistic book**
To construct the long-tail and heavy top scenario, seed 10–15 active levels each side. That aligns with the common practice of modeling arrival/cancellation behavior near the first 10–15 levels and gives you enough depth to test one-level and multi-level sweeps.

Use a depth distribution like this:
1. 70% of adds in levels 0–3 from the touch
2. 25% in levels 4–10
3. 5% in levels 11–15

That exact split is a design choice, but it is motivated by the empirical near-touch concentration plus long tail.

**Phase B: generate a mixed “cross” trace**
A good default mix is:
- 50% passive add (no match)
- 30% one-level match
- 20% multi-level sweep

The "good" is not “the market truth” it is a good benchmark workload because it gives you coverage over the major hot paths.

**Replenishment Rule**
After each generated event, if one side drops below a minimum number of levels, insert a few passive replenishment before continuing. 

## lobd QA

### Using `std::signal` for RAII

### State Hash
FNV-1a
- fast;
- Low Expected Collisions: The 64-bit FNV-1a boasts an extremely low collision rate, approximately 2^(-64)

FNV-1a is sequential: each byte updates the running state, so changing byte order changes intermediate states and therefore the final hash.
So if you change any of these, hash changes:

reorder bids/asks traversal,
reorder orders inside a level,
reorder fields within an order,
switch byte endianness.
That is also why this works as a **deterministic fingerprint: stable input order gives stable hash**.


difference between `ReplayCounters ctr;` and `ReplayCounters ctr{};`

## Subtle

1. What is "frame pointer" in add_compile_options(-fno-omit-frame-pointer)?
2. shoud not add `static` specifier to anonymous namespace members in C++?
