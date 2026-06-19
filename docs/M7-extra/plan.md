Problem 1: ex2gw is saturated over 96% in w2w decomp.
Question:
1. Why gw2lob not saturated?
2. How to investigate to resolve the saturation?

Problem 2: negatives exist in cross-process interval latencies.
Hypothesis: It's introduced by preemption jitter. It can happen when the order sent from gw is arriving lob, `lob_recv` is recorded, but gw is already be descheduled after sending and before recording `gw_after_send`. After isolcpu, all negatives should disappear in aln interval latencies.

Problem 3: timestamp instrumentation itself is a noise.
Question:
1. How to measure the cost of timestamp call?