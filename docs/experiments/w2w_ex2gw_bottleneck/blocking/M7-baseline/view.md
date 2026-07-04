## Headline

The stats data presented in center ± spread is `median ± mad`.
> Two families, `mean-std-cv` and `med-mad-robustc cv`, to stat (center ± spread) and data variance.
> We (me) choose the `med` family because:
> 1. `Mean` is more sensible to "bad tails" and "spikes" produced by corrupted runs, especially in a small sample. While `median` has 50% *Breakdown Point* to tolerate these corrupted runs data points.
> 2. `mean` is much efficient when the data is a clear Gaussian distribution. While with 10, 20 or 30 data points, we can not find a symmetric bell curve.

**engine cyc/msg**

Cross-mode delta is guarded by **bootstrapping**.

Match/decode/null produce three experimental sets under each scenario. They're used to attribute perf counters to engine by calculating the deltas between two sets per counter. And we must go with **CI** to calculate the uncertainty when dealing with deltas.

There is a closed-form formula `Welch's t-test` for `mean` to calculate CI. But the above two arguments about `mean` is still valid here. So we continue with median for deltas.

Hodges-Lehmann CI is a CI for difference between two sets, it calculates the diff of every possible elem pair between two sets to get a `n * n` difference array, then return the median. But it assumes that the two sets have the same distribution. With 10, 20 or 30 samples, it's hard to define a shape.

Bootstrapping drops all assumptions.

CI is presented in `point estimate, % CI [low, hi]`, where point est is `median delta`.

| scenario   | throughput(M/s)   | p99_w2w(ms)   | p99_lob_apply(ns)   | lob_engine cyc/msg, 95% CI       | lob_codec cyc/msg, 95% CI        | floor cyc/msg   |
|:-----------|:------------------|:--------------|:--------------------|:-------------------------|:-------------------------|:----------------|
| cross      | 0.27 ± 0.01       | 1.13 ± 0.08   | 2437.50 ± 18.50     | 437.02, [266.61, 592.61] | 58.70, [-97.89, 187.39]  | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 1.10 ± 0.05   | 2461.00 ± 10.50     | 664.31, [522.49, 754.64] | 61.90, [7.40, 166.64]    | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 1.33 ± 0.12   | 2416.00 ± 48.00     | 472.41, [299.53, 648.51] | 32.25, [-106.81, 209.10] | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3773.15   |     56.085 |       0.023 | 0.59%   |
| gw_intvl_decode_mean_stat  |       70.728  |      0.612 |       0.013 | 0.01%   |
| gw_intvl_send_mean_stat    |     2435.87   |     27.338 |       0.017 | 0.38%   |
| lob_intvl_decode_mean_stat |       69.2888 |      0.435 |       0.01  | 0.01%   |
| lob_intvl_apply_mean_stat  |      229.124  |      2.325 |       0.016 | 0.04%   |
| ex2gw_trans_mean_stat      |   614271      |   9558.26  |       0.024 | 96.59%  |
| gw2lob_trans_mean_stat     |    14724.3    |   2406.8   |       0.252 | 2.32%   |
| w2w_mean_stat              |   635926      |  11596.8   |       0.028 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3755.14   |     42.669 |       0.018 | 0.59%   |
| gw_intvl_decode_mean_stat  |       70.1479 |      0.291 |       0.006 | 0.01%   |
| gw_intvl_send_mean_stat    |     2422.31   |     22.825 |       0.015 | 0.38%   |
| lob_intvl_decode_mean_stat |       68.8734 |      0.31  |       0.007 | 0.01%   |
| lob_intvl_apply_mean_stat  |      264.851  |      2.568 |       0.015 | 0.04%   |
| ex2gw_trans_mean_stat      |   611479      |  13514.3   |       0.034 | 96.71%  |
| gw2lob_trans_mean_stat     |    14719.3    |   2282.11  |       0.239 | 2.33%   |
| w2w_mean_stat              |   632282      |  16651.1   |       0.041 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3837.63   |     31.056 |       0.012 | 0.60%   |
| gw_intvl_decode_mean_stat  |       72.5013 |      0.482 |       0.01  | 0.01%   |
| gw_intvl_send_mean_stat    |     2489.33   |     50.179 |       0.031 | 0.39%   |
| lob_intvl_decode_mean_stat |       71.1702 |      0.492 |       0.011 | 0.01%   |
| lob_intvl_apply_mean_stat  |      242.266  |      2.349 |       0.015 | 0.04%   |
| ex2gw_trans_mean_stat      |   621238      |  10501.6   |       0.026 | 96.70%  |
| gw2lob_trans_mean_stat     |    15476.4    |   1007.43  |       0.1   | 2.41%   |
| w2w_mean_stat              |   642421      |  10843.9   |       0.026 | 100.00% |
## Noise Floor

> Unbiased Robust CV. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2437.5         |    18.5 |       0.012 |     55.5 |       0.036 |
| w2w_p99_stat             |      1.13483e+06 | 77379.5 |       0.105 | 232139   |       0.315 |
| throughput_stat          | 270496           | 13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2461           |    10.5 |       0.007 |     31.5 |       0.021 |
| w2w_p99_stat             |      1.09663e+06 | 51309   |       0.072 | 153927   |       0.216 |
| throughput_stat          | 273362           | 14446   |       0.081 |  43338   |       0.243 |
### cancel
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2416           |     48   |       0.031 |    144   |       0.093 |
| w2w_p99_stat             |      1.33062e+06 | 119204   |       0.138 | 357611   |       0.414 |
| throughput_stat          | 271992           |  17464.5 |       0.099 |  52393.5 |       0.297 |
