## Observe - What's wrong?
I build two measurement tools - stage timestamps and perf counter attribution. And I built a w2w decomposition table to find the bottleneck in the w2w.

In the [M7-baseline](../../artifacts/summary/M7-baseline/view.md) run, The `ex2gw` is the largest interval in the chain and dominates over 96% (614 us) latency portion in the entire trip in all three scenarios. While another UDS trans `gw2lob` is normal, and the matching engine (lob_apply) stage is about invisible (0.23 us).
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3773.15   |     56.085 |       0.023 | 0.59%   |
| gw_intvl_decode_mean_stat  |       70.728  |      0.612 |       0.013 | 0.01%   |
| gw_intvl_send_mean_stat    |     2435.87   |     27.338 |       0.017 | 0.38%   |
| lob_intvl_decode_mean_stat |       69.2888 |      0.435 |       0.01  | 0.01%   |
| lob_intvl_apply_mean_stat  |      229.124  |      2.325 |       0.016 | 0.04%   |
| **ex2gw_trans_mean_stat**      |   **614271**      |   9558.26  |       0.024 | **96.59%**  |
| gw2lob_trans_mean_stat     |    14724.3    |   2406.8   |       0.252 | 2.32%   |
| w2w_mean_stat              |   635926      |  11596.8   |       0.028 | 100.00% |


## Diagnosis
Both `ex2gw` and `gw2lob` measure the interval from "the upstream producer finished sending" to "the downstream consumer's recv retuned the msg". So this interval consists of `"mem copy from user space to kernel" + "the waiting time in the queue" + "mem copy from kernel to user space"`.

Check the message trajectory: 
```text
pos%    ex2gw(ns)    gw2lob(ns)
  0        66,380       52,365     ← gw2lob: warmup (cold, queue not yet filled)
 10       423,895        1,970
 20       928,870        2,458
 50       217,705        3,498
 90       655,739        2,146
100       585,421        5,726
```

## Hypothesis
### Why `ex2gw` blocked?
`gw` recv mem copy from kernel to user space is much slower than `ex` mem copy from user space tpo kernel. Why?


### Why it's asymmetric - `gw2lob` is also a UDS transportation on the same machine?
`gw` consumes msgs from the upstream and feed them to lob. The feed speed depends on how fast the `gw` can recv, and it should slower than or equal to the lob can recv.

## Experiment
### Backpressure
Sending messages in absolute ticks from `ex`, make it can not flooding the queue so remove queuing time from w2w latency.
```cpp
t0 = clock(CLOCK_MONOTONIC_RAW)
send()
while (clock(CLOCK_MONOTONIC_RAW) - t0 < T) {}
```
To find the `T`, frist we find the reference from `gw2lob`, whose `median` is `2.4 us`. Then we repeat experiments with `T = 1, 2, 3, 4`. And expect  `ex2gw_trans_mean_stat` should drop from `T = 1` to `2`, while no changes from `T = 3` to `4`. Then we know the minimum T falls between `2` and `3`.

#### Side effect
The throughput will be injected artificial latencies between messages and won't be useful.  
