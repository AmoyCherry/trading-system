## Q&A

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

## Subtle

1. What is "frame pointer" in add_compile_options(-fno-omit-frame-pointer)?
2. shoud not add `static` specifier to anonymous namespace members in C++?
