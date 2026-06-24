(1) saturation/offered-load, (2) jitter→isolation, (3) stride necessity, (4) pre-fault necessity, (5) phrasing.


Problem 1: ex2gw is saturated over 96% in w2w decomp.
Question:
1. Why gw2lob not saturated?
2. How to investigate to resolve the saturation?
My reasoning is, if gw2lob didn't saturate, ex2gw should also possible to be not saturated? If we can't rule out that, we can only reconstruct the w2w decomposition table?  

Problem 2: negatives exist in cross-process interval latencies.
Hypothesis: It's introduced by preemption jitter. It can happen when the order sent from gw is arriving lob, `lob_recv` is recorded, but gw is already be descheduled after sending msg and before recording `gw_after_send`. After isolcpu, all negatives should disappear in all interval latencies.

Problem 3: timestamp instrumentation itself is a noise.
Question:
1. How to measure the cost of timestamp call?
2. What magnitude is large enough for a timestamp overhead? Larger than the MDE?
3. If it's large enough, what approaches should we consider to apply ?

## What LLMs helped me?
1. Find my blind spots in my logic chain when inferencing root causes.
2. Literature review the possible techniques that may resolve my problems.