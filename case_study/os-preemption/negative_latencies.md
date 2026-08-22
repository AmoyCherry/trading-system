## Observe
`Analyzer` loads raw timestamps to calc latencies and their metrics then stats per interval. It has a guardrail to verify all intervals of all messages should be positive or zero.
But the guardrail reported some negative interval latencies in the log.
```text
scenario, mode, repeat, interval notes
cross, match, 1, ex2gw_trans has negatives.
cross, match, 1, gw2lob_trans has negatives.
cross, match, 2, ex2gw_trans has negatives.
...
cross, match, 9, gw2lob_trans has negatives.
cross, match, 10, ex2gw_trans has negatives.
cross, match, 10, gw2lob_trans has negatives.
add, match, 1, ex2gw_trans has negatives.
add, match, 1, gw2lob_trans has negatives.
add, match, 2, ex2gw_trans has negatives.
...
add, match, 10, gw2lob_trans has negatives.
cancel, match, 1, ex2gw_trans has negatives.
cancel, match, 1, gw2lob_trans has negatives.
cancel, match, 2, ex2gw_trans has negatives.
...
cancel, match, 10, gw2lob_trans has negatives.
```

## Diagnosis