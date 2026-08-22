## Q1 - Why the baseline drops 633.7 → 541.1 when switching to non-blocking `recvfrom`?

1. At baseline, the `ex2gw` interval latency consists of `waiting time in the queue` + `mem copy`.
2. A msg's waiting time is the sum of previous msgs' draining time.
3. The head msg's draining time is actually a msg iteration of gw: `mem copy + gw decode + blocking sendto`. The 2nd msg becomes new head once the head drained, so a msg's waiting time is `(queue_size - 1) * gw_iteration`.

So `ex2gw` can be influenced by two factors: queue size and gw_iteration. 

4. Blocking `recvfrom` of `lob` has 770,969 vol sw, added wake-up callback overhead to `gw`'s `sendto`. While polling `recvfrom` only has 0 vol sw so removes wake-up callback from `gw`'s `sendto`. As a result, 
   - The median of `gw sendto p50` drops from 2,173.5 → 1,889.5 ns, ~13.1%; and the median of `ex2gw` drops 633.7 → 541.1 us, ~14.6%.
   - `gw_intvl_decode_p50_stat` is nearly unchanged at 70.0 → 69.5 ns; `gw` almost never sleeps at saturation in either mode, with 691 vs 253 voluntary switches.

With `queue_size` and `gw:decode` keep unchanged, the changing scale of `gw:sendto` supports the wake-up work is the dominant explanation for the faster drain cycle.

> sendto() → wake_up_interruptible()


### Baseline Reconstruction

A. The queue size changed only slightly.

Compute every msg's queue size after arriving the queue: in `exts.csv` and `gwts.csv`, by binary searching every `ex_after_send[i]` in `gw_recv[]`, `queue_size[i] = i − #{j : gw_recv[j] ≤ ex_after_send[i]}`. 

Get the median `q50_outstanding` is blocking `162` vs polling `158` .

B. Reconstruct the `ex2gw` p50 latency.

`gw`'s mean receive-cycle gap is `gw_recv_tstamps(last − first) / (N − 1)`. It is `3.969us` for blocking and `3.437us` for polling.

- `162` × `3.969` = `643` vs `633.7us` measured
- `158` × `3.437` = `543` vs `541.1us` measured

This is not a per-message identity; it is a steady-state reconstruction that validates the queueing model and reproduces both the absolute scale.


## Q2 - Why does the floor-touch region move from `5-6us` to `4us` when switching to polling `recvfrom`?

- The saturation boundary is the T where the queue begins to empty.
- The p50-defined knee is the T where the arriving-alone msgs (the fast population) become dominant enough to report the p50 floor; but queues may still form in the tail.
- "Arriving alone" in `queue_census.py` means no older datagram is outstanding at `ex_after_send`. `gw` may still be decoding or sending the previously received message.

1. The saturation bound does not move: 
   - both blocking and polling collapsed from a large queue `159` vs `158.5` at `3us` to `1.5` vs `1` at `4us`; and `1` vs `1` at `5us`, for the p50 msgs.
2. What moves is the T where the fast population becomes dominant enough for `ex2gw` to report the floor:
   - Polling reaches the `p50` floor `1.1us` immediately at `T=4us`. Because `90.6%` of messages arriving alone without any older datagram outstanding, `55.4%` msgs in the arriving alone population are below that floor. And the overall p50 is approximately `50/90.6 = p55.2` of the fast population—a typical fast-path latency.
   - At blocking `T=4us`, the median of the `ex2gw` p50 is `3.99us`, and only `50.1%` of msgs arriving alone. `91.2%` msgs in the arriving alone population, and `8.8%` in the queued population are below that `3.99us`. The overall `ex2gw` p50 is in the overlapping area between the two populations, so the reported `3.99us` is not a typical fast path latency.

### Mechanism

When blocking `recvfrom` crosses the boundary, the queue begins to empty while queues still exist. So it starts introducing wake-up overhead to the current msg's `ex2gw` interval directly and delaying the head to be drained (give opportunity for the next msg to see a msg outstanding in the queue). Then the longer sending gap also increases `lob`'s voluntary switches (895,314 at T=4 vs 770,969 at baseline) so increases `gw_intvl_send_p50_stat` (2,341.5 ns at T=4 vs 2,173.5 ns at baseline), influencing the msg waiting time in the `ex2gw` queue. Combining these two mechanisms, small queues keep forming (blocking q50 1.5 vs polling 1), and only 50.1% of blocking messages arrive with no older datagram outstanding (they may still need to wait for `gw` to finish decoding and sending the previous message).



## Q3 - Why the p50 floor drops from `3.1us` to `1.1us` when switching to non-blocking `recvfrom`?

- By further increasing the T, per-msg sending gap is longer and receivers start waiting with blocking IO.
- `ex sendto` consists of `mem copy us2k` + `wake-up callback`; `ex2gw` consists of `most of wake-up` + `mem copy k2us`.
- Blocking `recvfrom` introduces the `callback` and `wake-up` overhead for both sides. While polling removes the wake-up overhead:
  - at `T=12us` vol sw `1,844,116` → `836` times; `ex_send p50` `2,668` → `2,057ns`; ex2gw p50 `3,481` → `1,142ns`.
  - at `T=6us`: `3,106` − `1,130` = `1.98µs`; `T=12`: `3,482` − `1,142` = `2.34µs`. The gaps are consistent with the mechanism: the longer the sending gap, the longer the receiver wait, the more likely the deeper idle states which cost more. The `gw`'s vol sw drops `0.81M` → `1052` at T=6us; `1.84M` → `836` at `T=12`.

### idle states side effects

`lob_apply` p50 (ns) — blocking vs non-blocking (the positive control), shown as `median ± MAD`:

| T (µs)       |   4 |   5 |   6 |  12 |
|:-------------|----:|----:|----:|----:|
| blocking     | 152 ± 2 | 158 ± 1 | 182 ± 2 | 210 ± 6 |
| non-blocking | 138 ± 1 | 138 ± 1 | 138 ± 0 | 139 ± 1 |

The matching engine `median(lob_intvl_apply_p50_stat)` increases from `0.15 us` to `0.21 us` when using blocking `recvfrom`. That interval is a pure in-process work after `recvfrom` has returned, so it does not include socket queueing or the direct blocking wait. When switching to non-blocking `recvfrom`, the core stays hot and the median of `lob_apply` p50 stays flat.

This positive control supports the idle states side effects, but it does not split cold-down/post-idle frequency settling.
