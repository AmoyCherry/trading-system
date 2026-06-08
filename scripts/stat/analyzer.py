"""
1. Guard for each repeat
Read gateway.log to assert decode and seq errors are 0
Read all three .csv and assert the same count, min seq and max seq

Calc in- and cross-proc intervals within each repeat. Then calc these across repeats:
- mean
- MAD (the noise floor, socalled basic bg noise)
- CV=stddev/mean (normalized noise)

2. Process csv
Compute intervals:
- ex:
    - ex_intvl_send = after_send - before_send
- gw:
    - gw_intvl_decode = gw_before_send - gw_recv;
    - gw_intvl_send = gw_after_send - gw_before_send
- lobs:
    - lob_intvl_decode = lob_decode_done - lob_recv;
    - lob_intvl_apply = lob_apply_done - lob_decode_done
- ex2gw_trans = gw_recv − ex_after_send
- gw2lob_trans = lob_recv − gw_after_send
- w2w = lob_apply_done − ex_before_send
NOTE: Cross-process sub is only valid when they run on the same machine. On different machines this needs clock sync.
And store them in separated arrays, like gw_intvl_decode[], then sort these arrays.

Question: there are other results from repeat runs, so how to organize them and then calculate percentiles? Such as gw_intvl_decode[repeat_idx][seq_idx]?
Question: is it better to store structs like {seq, interval, ...}? Do we need seq accompany every interval?

You said "Aggregate across the 5 repeats per cell". You mean calculate median + MAD/stddev + CV by 5 of the all repeats? Then how about the remainder repeats? What's each of these (median + MAD/stddev + CV) used for?

3. Process perf stat
Compute Mean/MAD/CV among repeats of each cell.
Read perf_stat.log. Then calculate:
- cycles/msg, get msgs from lobd.log
- branch-miss-rate
- cache-miss-rate
- Instr/Cycle can tell an op drops the cache-miss-rate and inc the IPC to reduce the CPU stalls

4. Cross-mode deltas
Question: wire-to-wire latency decomposition is used against perf_stat? And why we do this and what insights we want to get from cross-mode deltas?


"""

import numpy as np


SCENARIOS = ["cross", "add", "cancel"]
MODES = ["match", "decode", "null"]

"""
cells
for s in SCENARIOS:
    for m in MODES:
        for i in REPEATS:
            repeat = load_repeat(s, m, i)
            cells[s][m].repeats[i] = process_repeat(repeat)
        calc_stats(cells[s][m])
        
save(cells)
"""











