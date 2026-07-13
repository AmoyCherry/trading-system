## Headline

The stats data presented in `center ± spread` is `median ± mad`.
> Two families, `mean-std-cv` and `med-mad-robustc cv`, to stat (center ± spread) and data variance.
> We (me) choose the `med` family because:
> 1. `Mean` is more sensible to "bad tails" and "spikes" produced by corrupted runs, especially in a small sample. While `median` has 50% *Breakdown Point* to tolerate these corrupted runs data points.
> 2. `mean` is much efficient when the data is a clear Gaussian distribution. While with 10, 20 or 30 data points, we can not find a symmetric bell curve.

**`engine cyc/msg`**

Cross-mode delta is calculated by `med(match) - med(decode)` and is guarded by **CI**. The CI is calculated by  **bootstrapping**.

To calculate the deltas between two exprtl sets per counter, we must go with CI to calculate the uncertainty.

- The above two arguments about `mean` is still valid here. So we continue with median for deltas.
- Hodges-Lehmann CI is a CI for differences between two sets, it calculates the diff of every possible elem pair between two sets to get a `n * n` difference array, then return the median. But it assumes that the two sets have the identical shape and only different in location shift. With 10, 20 or 30 samples, it's even hard to define a shape.

| scenario   | throughput(M/s)   | p99_w2w(ms)   | p99_lob_apply(ns)   | lob_engine cyc/msg, 95% CI   | lob_codec cyc/msg, 95% CI   | floor cyc/msg   |
|:-----------|:------------------|:--------------|:--------------------|:-----------------------------|:----------------------------|:----------------|
| cross      | 0.29 ± 0.00       | 1.25 ± 0.14   | 2488.00 ± 21.50     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.21 ± 0.15   | 2551.50 ± 10.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 1.21 ± 0.09   | 2412.00 ± 20.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2828.47   |     90.054 |       0.049 | 0.44%   |
| gw_intvl_decode_mean_stat  |       72.2774 |      0.564 |       0.012 | 0.01%   |
| gw_intvl_send_mean_stat    |     2486.17   |     38.093 |       0.024 | 0.39%   |
| lob_intvl_decode_mean_stat |       70.0323 |      0.855 |       0.019 | 0.01%   |
| lob_intvl_apply_mean_stat  |      227.439  |      1.58  |       0.011 | 0.04%   |
| ex2gw_trans_mean_stat      |   624831      |  10168.1   |       0.025 | 96.85%  |
| gw2lob_trans_mean_stat     |    15851.5    |   2998.34  |       0.291 | 2.46%   |
| w2w_mean_stat              |   645143      |  12878.8   |       0.031 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2852.87   |    111.711 |       0.06  | 0.44%   |
| gw_intvl_decode_mean_stat  |       72.5713 |      1.31  |       0.028 | 0.01%   |
| gw_intvl_send_mean_stat    |     2503.96   |     38.33  |       0.024 | 0.38%   |
| lob_intvl_decode_mean_stat |       70.1447 |      1.059 |       0.023 | 0.01%   |
| lob_intvl_apply_mean_stat  |      265.616  |      3.66  |       0.021 | 0.04%   |
| ex2gw_trans_mean_stat      |   632507      |  11335.6   |       0.028 | 96.88%  |
| gw2lob_trans_mean_stat     |    15702.5    |   2730.57  |       0.268 | 2.41%   |
| w2w_mean_stat              |   652901      |  14476.7   |       0.034 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2835.87   |     47.145 |       0.026 | 0.43%   |
| gw_intvl_decode_mean_stat  |       73.5729 |      0.677 |       0.014 | 0.01%   |
| gw_intvl_send_mean_stat    |     2499.36   |     23.454 |       0.014 | 0.38%   |
| lob_intvl_decode_mean_stat |       71.1366 |      0.569 |       0.012 | 0.01%   |
| lob_intvl_apply_mean_stat  |      234.785  |      1.887 |       0.012 | 0.04%   |
| ex2gw_trans_mean_stat      |   631292      |   6891.43  |       0.017 | 96.78%  |
| gw2lob_trans_mean_stat     |    16097      |   1928.7   |       0.185 | 2.47%   |
| w2w_mean_stat              |   652291      |   9685.82  |       0.023 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2488           |     21.5 |       0.013 |     64.5 |       0.039 |
| w2w_p99_stat             |      1.25473e+06 | 140602   |       0.173 | 421807   |       0.519 |
| throughput_stat          | 290018           |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2551.5         |     10.5 |       0.006 |     31.5 |       0.018 |
| w2w_p99_stat             |      1.20552e+06 | 150656   |       0.193 | 451968   |       0.579 |
| throughput_stat          | 289630           |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2412           |    20   |       0.013 |     60   |       0.039 |
| w2w_p99_stat             |      1.21003e+06 | 92729   |       0.118 | 278187   |       0.354 |
| throughput_stat          | 290176           |  4163.5 |       0.022 |  12490.5 |       0.066 |
