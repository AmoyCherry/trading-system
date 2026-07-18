> Headline:
> 
> The apparent 633 µs transport is queueing time; the queue-free p50 latency is 3.1 µs blocking / 1.1 µs busy-poll; busy-poll keeps the consumer cores hot and removes most blocking wake-up jitter.
> 
> setup:
> 
> - 2M msgs × 10 repeats × 3 scenarios
> 
> - pinned P-cores, no turbo
> 
> - median±MAD, MDE gate

## Observe - What's wrong?
I build two measurement tools - stage timestamps and perf counter attribution. And I built a w2w decomposition table to find the bottleneck in the w2w.

In the [M7-baseline](./blocking//M7-baseline) run, The `ex2gw` is the **largest interval** and **dominates over 96% (614 us)** latency portion in the entire trip in all three scenarios. While another UDS trans `gw2lob` is normal, and the matching engine (lob_apply) stage is about invisible (0.23 us).
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
- `gw` needs to process a decode and a `sendto` syscall between two recv calls; while `ex` can send a msg immediately after the  previous msg. The consumer is much slower than the producer, so the queue size of the socket buffer can be gradually increased.
- The `ex2gw` measures the interval from "the upstream producer finished sending" to "the downstream consumer's `recvfrom` returned the msg". So this interval consists of `"waiting in the socket buffer" + "receiver wake-up" + "mem copy k2us" `.
- Finally, the interval latency is actually measuring the waiting time.

## Hypothesis

By controlling the msg sending speed of `ex` from fast to slow, like from `1us` to `10us`, the `ex2gw` latency should decrease to a level then keep unchanged or keep flat. 

The level is where the queue size keeps <= 1, so msgs arrive without any older datagram outstanding in the socket buffer.

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

The table shows `median ± MAD` of `ex2gw_trans_{metric}_stat` (the same reason to the Headline).

For the within-run metric, I pick up the `p50` to analyze. Because the question I want to answer is "Whether messages pile up waiting in line at the gateway's socket buffer (the queue). At saturation that mechanism influence almost every message". So I need the typical latency, which should be `p50`.
- `p99` and `max` only consider the few msg at the tail, not the typical latency that can reflect "almost every msg influenced by something"
- `mean` also can not answer the question. Although it's calculated by 2 million msg latencies, that just makes `mean` value precisely absorbs both the typical latencies and the tail. So it's not the pure typical latency, the `mean` value folded tail mechanism into the number such as OS preemption which I didn't enable `isolcpus` during this experiment.  

> Runs with `getrusage` ctx-sw brackets (the old T=7–11 sweep dirs are retired; this table's baseline is the same-era rerun — M7-baseline stays as the Observe reference above). Source: [baseline](./blocking/baseline-ctxsw), [T=3](./blocking/3us-ctxsw), [T=4](./blocking/4us-ctxsw), [T=5](./blocking/5us-ctxsw), [T=6](./blocking/6us-ctxsw), [T=12](./blocking/12us-ctxsw).

| T (µs)   | p50 (ns) (typical) | p99 (ns)       | max (ns)           | mean (ns)      |
|:---------|-------------------:|---------------:|-------------------:|---------------:|
| baseline | 633,722 ± 16,378 | 1,179,895 ± 109,600 | 30,834,730 ± 411,268 | 640,181 ± 17,275 |
| 3        | 619,464 ± 6,784  | 1,152,403 ± 94,486  | 30,842,866 ± 822,336 | 624,831 ± 10,168 |
| 4        |     3,994 ± 408  | 1,036,002 ± 184,978 | 29,431,590 ± 126,463 | 54,828 ± 21,913 |
| 5        |     3,197 ± 114  | 1,246,046 ± 236,994 | 28,832,835 ± 327,458 | 55,241 ± 36,280 |
| 6        |      3,106 ± 44  |   644,499 ± 216,993 | 28,979,369 ± 236,464 | 31,465 ± 5,783 |
| 12       |      3,482 ± 67  |   537,838 ± 349,423 | 26,992,071 ± 351,462 | 28,965 ± 9,648 |


### Conclusion
**Matches hypothesis**

The typical time of `ex2gw` dropped from the baseline `~633us` to the lowest `~3.1 us` and stayed above there, where the queue was removed.

The p-50 defined knee is around `5-6us`, where the empty-queue becomes dominant enough for p50, and the median of `ex2gw` `p50` latency drops to its floor `3.1 us`.

**Not matches hypothesis**

After the p50-defined knee around `5-6us`, `median(ex2gw_trans_p50_stat)` did not stay perfectly flat; it increased to `3.48us`.

- Further larger producing gaps let downstream consumers wait longer between messages.
- With blocking `recvfrom`, the consumer enters the kernel and can sleep until the next message arrives. This introduces wake-up overhead on both end sender and the receiver.
- If no other runnable work is on that CPU, the kernel can run the idle task and enter idle power states. The larger the sending gap, the more likely the consumer gets an idle-state side effects including post-idle frequency settling and even cache coldness.

> Note, when using cpu pinning + isolcpus for a single-threaded process, the cpu can still run the idle task (pid = 0).

Verify: I was indeed using blocking `recvfrom` initially because all three processes are single-threaded. Then I ran an A/B test using polling `recvfrom` (`MSG_DONTWAIT`) with blocking `sendto`.

### `cross` results — non-blocking `recvfrom` (positive control)

Same pacing sweep; `gw`/`lob` use non-blocking `recvfrom` (`MSG_DONTWAIT`, busy-wait) so the consumer never sleeps and the core stays hot.

> Source: [baseline](./non-blocking/baseline-ctxsw-20260710_154811), [T=3](./non-blocking/3us-ctxsw-20260710_165055), [T=4](./non-blocking/4us-ctxsw-20260710_161042), [T=5](./non-blocking/5us-ctxsw-20260712_201952), [T=6](./non-blocking/6us-ctxsw-20260710_162602), [T=12](./non-blocking/12us-ctxsw-20260710_164224).

| T (µs)   | p50 (ns) (typical) | p99 (ns)     | max (ns)           | mean (ns)     |
|:---------|-------------------:|-------------:|-------------------:|--------------:|
| baseline | 541,134 ± 6,974 | 1,034,412 ± 86,066 | 30,208,071 ± 326,527 | 536,993 ± 9,243 |
| 3        | 541,832 ± 12,897 | 957,145 ± 46,942  | 30,435,931 ± 179,788 | 549,850 ± 14,125 |
| 4        |   1,129 ± 8     | 932,107 ± 142,385 | 29,471,664 ± 183,766 | 36,752 ± 14,334 |
| 5        |   1,141 ± 12    |  14,279 ± 12,319  | 28,721,038 ± 162,332 | 9,887 ± 1,599 |
| 6        |   1,130 ± 13    |   1,789 ± 89      | 28,594,476 ± 78,482  | 8,518 ± 589 |
| 12       |   1,142 ± 8     |   1,771 ± 27      | 26,914,427 ± 175,102 | 6,663 ± 139 |


### Rusage

I measured both the voluntary and involuntary ctx sw by `getrusage` to close this cs.

> Source: [blocking-ctxsw](blocking/12us-ctxsw/perf_summary.csv) and [non-blocking-ctxsw](./non-blocking/12us-ctxsw-20260710_164224/perf_summary.csv) (post-bugfix rerun)

At `T = 12us`, the `median±MAD`:

| IO mode             |  `gw` voluntary ctx sw | `gw` involuntary ctx sw | `lob` voluntary ctx sw | `lob` involuntary ctx sw |
|:--------------------|-----------------------:|------------------------:|-----------------------:|-------------------------:|
| blocking `recvfrom` | 1,844,116.5 ± 60,783.5 | 137.0 ± 34.0 | 1,858,788.5 ± 62,994.5 | 140.5 ± 32.0 |
| polling `recvfrom`  |         836.0 ± 28.5   | 54.0 ± 3.0 | 0.0 ± 0.0 | 298.5 ± 24.0 |

- Blocking `recvfrom`'s `voluntary ctx sw` vs polling is  1.84M vs 800 for 2 million msgs. That's approximately 0.92 vol sw per msg, consistent with the `gw` sleeping between most receives. And polling removes the wake-up overhead for most of the msgs. 
- The `involuntary ctx sw` is smaller than 4 hundreds in both IO modes, that's far smaller than 1% of messages. That means `involuntary ctx sw` operations including OS preemption is very rare and can only influence the tails such as `p99/max`, and can not move `p50`.

### Blocking vs Polling Difference

> [qas.md](./qas.md)

1. baseline: `lob`'s blocking `recvfrom` adds wake-up overhead to `gw sendto` - add waiting time in `ex2gw`;
2. floor-touch region: smalls queues keep forming when crossing the saturation bound, so arriving alone rate are much slower vs polling around the bound. (`90.6%` vs `50.1%` at `T=4`)
3. floor: blocking `recvfrom` introduce wake-up overhead on both sides.

## Pacing side effect

To measure the throughput to answer "how fast the `ex` can send", we should remove the pacing.

With pacing, the throughput can only use to check if the pacing worked. Because we can calculate the throughput before running, such as `83K msgs/sec = 1 sec / 12 us` when pacing gap is `12us`.
