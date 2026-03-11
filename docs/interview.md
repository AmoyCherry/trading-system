## What's important in interviews?
The process of case study:
I can 
1. identify a realistic trading-system bottleneck (define the problem and propose a solution), 
2. design a clean experiment around it, measure it properly, 
4. and explain the trade-off like an engineer who could work on production systems.

expert C++, low-level systems knowledge, 
1. CPU-architecture awareness, 
2. optimization across abstraction layers, 
3. and judgment about latency, throughput, simplicity, maintainability, and behavior under pressure


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


## Subtle

1. What is "frame pointer" in add_compile_options(-fno-omit-frame-pointer)?
2. shoud not add `static` specifier to anonymous namespace members in C++?
