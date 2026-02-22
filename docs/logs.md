## Feb 21, 2026 at 11:58
1. Understand console log of micro and perf with the highest ROI.
   
   perf:
   - UserCounters is custom field. Here we measure processed orders per second by `state.SetItemsProcessed`.
   - Iterations are iteration times of tese loop `for (state)`.
   - Time is real world elapsed time including mem latency. CPU is CPU time.
   
   bm:
   - Mean: The arithmetic average.
   - Median: 50% of runs were faster than this.
   - Stddev: Standard Deviation: measures how much your results vary.
   - CV (%): Coefficient of Variation (stddev/mean). Jitter Metric. For low-latency code, you want this under 1%.
2. Understand save files of micro and perf with the highest ROI.
   Golden metrics at application level:
   - Orders processed per second.
   
   Golden metrics at Machine level:
   - Instructions per Cycle (IPC).
   - Cache-references / Cache-misses.
   - Branches / Branch-misses.
3. Can cache-misses be supported in perf_stat.txt? 
   Elevate `perf_event_paranoid`. Physical machine must have support.
4. Address M3 deliverables:
```text
When you can do these, M3 is done:
- ./scripts/run_micro.sh produces microbench.json
- ./scripts/perf_stat_micro.sh BM_MatchSweep produces perf_stat.txt
- You tag a commit: git tag v0-microbench-baseline
- You can explain (in 60 seconds):
    - what each benchmark measures
        The performance of adding/canceling/matching orders. We care about processed orders ps, mean/median/stddev/cv on the application level; IPC/cache/branch on the machine level.
    - why setup is excluded
    - what 2 perf counters you look at first and why
```
5. Also save perf console log to artifacts.
   Use `COMMAND | tee PATH`
6. Understand why add executable first?
   Define the taget so you can have a reference to add options and link libraries to the target.
7. Tag baseline.


## Feb 20, 2026 at 22:19
1. Built ts on ubuntu ARM.
2. Run micro and perf.

## Feb 8, 2026 at 21:42
1. Implement order book;
2. Add UTs;

Learned:
1. `std::variant`

## Feb 6, 2026 at 19:28

Ask 5.2 pro:
```text
Actually, I'm not very familiar with C++ syntax. Do I need to write the code of milestone 1 line by line manually? There are two options for me.

## Option 1
I can just copy them that you just provided. And I follow the previous methodology that we focus on benchmark-driven performance case study for this project. For C++ grammar I learn when needed and only for the essential parts. I can learn uncovered grammar by other projects like implement string or vector or shared pointers. But for this project, we only focus on the small correct core + measurement. 

## Option 2
I also consider maybe it's better to write them line by line so that I can have comprehensive familiarity with the code, and the code is not too much to write. And as the plan, we take 1-1.5 weeks for milestone 1, should it better that you also provide the pseudocode code, so I develop against the pseudocode and specs,and I can do self-verify by your provided final code. 
```

Use Option 1.5: only use the loop when crucial. Learn grammar adaptively.

## Feb 5, 2026 at 18:43

Initial Milestones.
Confirm file tree.
Add CMakeLists.txt
