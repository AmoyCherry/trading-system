> Headline:
> 
> The apparent 614 µs transport was queueing time; true transport is 3.0 µs blocking / 1.17 µs busy-poll; the engine is 134–218 ns; busy-poll avoids cache cold down due to idle.
> 
> setup:
> 
> - 2M msgs × 10 repeats × 3 scenarios
> 
> - pinned P-cores, no-turbo, 
> 
> - median±MAD, MDE gate

## Observe - What's wrong?
I build two measurement tools - stage timestamps and perf counter attribution. And I built a w2w decomposition table to find the bottleneck in the w2w.

In the [M7-baseline](../../artifacts/summary/M7-baseline/view.md) run, The `ex2gw` is the largest interval and dominates over 96% (614 us) latency portion in the entire trip in all three scenarios. While another UDS trans `gw2lob` is normal, and the matching engine (lob_apply) stage is about invisible (0.23 us).
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
Both `ex2gw` and `gw2lob` measure the interval from "the upstream producer finished sending" to "the downstream consumer's recv returned the msg". So this interval consists of `"mem copy from user space to kernel" + "the waiting time in the socket buffer" + "mem copy from kernel to user space"`.

`gw` needs to process a decode and a `sendto` syscall between two recv calls; while `ex` can send a msg immediately after the  previous msg. If the mem copies have the close efficiency, the queue can be gradually increased.

Finally, I'm actually measuring the waiting time rather than the real transportation latency.

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

By controlling the msg sending speed of `ex` from fast to slow, like from 1 us to 10 us, the `ex2gw` latency should decrease to a level then keep unchanged. 

The level is where the queue size keeps <= 1, so every msg can be processed immediately by the consumer after arriving. So that no queuing time to be recorded in the interval.

### Why it's asymmetric - `gw2lob` is also a UDS transportation on the same machine?
`gw` consumes msgs from the upstream and feed them to lob. The feed speed depends on how fast the `gw` can recv, and it should slower than or equal to the lob can recv.

## Experiment
### Strategy - Pacing
Sending messages in absolute ticks from `ex`.
```cpp
t0 = clock(CLOCK_MONOTONIC_RAW)
sendto()
while (clock(CLOCK_MONOTONIC_RAW) - t0 < T) {}
```
To find the start point of the pacing gap, considering the `gw` consists of `recv + decode + send`. We can use `median(gw_intvl_decode_p99) = 109`  + `median(gw_intvl_send_p99) = 4464` = `4573 ns` as a lower-bound. So the experiments can start from `4 us`.  

### `cross` results

The table shows `median` of `ex2gw_trans_{metric}_stat` (the same reason to the Headline). 

For the within-run metric, I pick up the `p50` to analyze. Because the question I want to answer is "Whether messages pile up waiting in line at the gateway's socket buffer (the queue). At saturation that mechanism influence almost every message". So I need the typical latency, which is `p50`.
- `p99` and `max` only consider the few msg at the tail, not the typical latency that can reflect "almost every msg influenced by something"
- `mean` also can not answer the question. Although it's calculated by 2 million msg latencies, that just makes `mean` value precisely absorbs both the typical latencies and the tail. So it's not the pure typical latency, the `mean` value folded tail mechanism into the number such as OS preemption which I didn't enable `isolcpus` during this experiment.  

| T (µs)   | p50 (ns) (typical) | p99 (ns)  | max (ns)    | mean (ns) |
|:---------|-------------------:|----------:|------------:|----------:|
| baseline |           ~613,000 | 1,073,870 | ~30,000,000 |   614,271 |
| 4        |              4,001 | 1,100,889 |  28,993,319 |    60,357 |
| 6        |              3,063 |    89,868 |  28,508,483 |    17,654 |
| 7        |              3,002 |    37,928 |  28,258,058 |    16,825 |
| 8        |              3,366 |   129,246 |  28,124,868 |    19,736 |
| 9        |              3,096 |    71,669 |  27,564,175 |    19,834 |
| 10       |              3,311 |    20,747 |  27,430,808 |    16,232 |
| 11       |              3,366 |     7,814 |  27,258,212 |    13,583 |
| 12       |              3,357 |     8,198 |  27,089,716 |    14,011 |


### `cross` results — non-blocking `recvfrom` (positive control)

Same pacing sweep; `gw`/`lob` use non-blocking `recvfrom` (`MSG_DONTWAIT`, busy-wait) so the consumer never sleeps and the core stays hot.

| T (µs)   | p50 (ns) (typical) | p99 (ns) | max (ns)    | mean (ns) |
|:---------|-------------------:|---------:|------------:|----------:|
| baseline |            532,395 |  919,890 |  30,286,566 |   526,635 |
| 4        |              1,168 |  774,208 |  29,471,004 |    23,926 |
| 6        |              1,163 |    1,721 |  28,856,102 |     8,835 |
| 7        |              1,166 |    1,714 |  29,235,374 |     8,376 |
| 9        |              1,170 |    1,690 |  27,546,572 |     7,170 |
| 11       |              1,174 |    1,665 |  27,264,738 |     6,752 |
| 12       |              1,168 |    1,656 |  26,866,969 |     6,536 |

`lob_apply` p50 (ns) — blocking vs non-blocking (the positive control):

| T (µs)       |   4 |   6 |   7 |   8 |   9 |  10 |  11 |  12 |
|:-------------|----:|----:|----:|----:|----:|----:|----:|----:|
| blocking     | 156 | 190 | 191 | 198 | 216 | 214 | 218 | 218 |
| non-blocking | 134 | 134 | 135 |  —  | 134 |  —  | 135 | 136 |

### Conclusion
**Matches hypothesis**

The typical time of `ex2gw` dropped from the baseline `~613 us` to the lowest `~3 us` and stayed above there, where the queue was removed.

The knee is around `6us` where the median of `ex2gw` `p50` latency drops to `3.0 us`.

**Not matches hypothesis**

`median(ex2gw_trans_p50_stat)` jumped to `~3.3 us`, when continuously increasing the sending gap after `T = 7us`.

After the queuing time was removed at `T = 7us`, further larger producing gap lets downstream consumers have more time to wait msgs. When using blocking `recvfrom` that enters the kernel and sleeps until a new msg arrives, if no other runnable work is on the cpu, the kernel can run the idle task and enter C-States. The larger sending gap, the longer sleeping time in kernel if using blocking IO, and more change to enter the deeper idle power states (C-States). When entering C3 and above, the cache will be flushed. Hot cache lines can be evicted.

> Note, when using cpu pinning + isolcpus for a single-threaded process, the cpu can still run the idle task (pid = 0).


Verify: I was indeed using a blocking `recvfrom` initially because all three processes are single-threaded process. Then I used an A/B test that using non-blocking `recvfrom` (with flag `MSG_DONTWAIT`) with blocking `sendto`.

------

> Why the transport dropped from `3` to `1.2` when changing blocking `recvfrom` to non-blocking.
> 
>  It's the wake-up time around 1.8 us?

> Why the jump from `3.0 us` to `3.3` is not because longer sleeping time causes more OS preemption?
>
> OS preemption on `gw` can cause cache flushing and form a small queue. But it is rare and can only affect the tails such as `p99/max`, cannot move `p50` unless the OS preemption can affect `>50%` of messages. 
> 
> The recorded throughput in `ex` is `~260K/sec`, which is `~3.8 us/msg`, to affect over 50% msgs, the OS preemption frequency should be faster than `7.8 us`, so that processing two msg in `ex` will have at least one msg encounter an OS preemption.

The matching engine in lob has a similar effect under a larger sending gap. `median(lob_intvl_apply_p50_stat)` increases from `0.15 us` to `0.21 us` smoothly. By using non-blocking `recvfrom`, the medians of the matching engine percentile latencies keep unchanged which conform to my hypothesis. 

Waiting in the queue, idle power states and cache coldness are two distinct mechanism. This case study isolated them by changing one variable, pacing gap or polling at a time.

#### Side effect
The throughput will be injected artificial latencies between messages and won't be useful.