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
| cross      | 0.29 ± 0.00       | 0.46 ± 0.41   | 2374.50 ± 7.00      | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.49 ± 0.12   | 2419.00 ± 11.00     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.86 ± 0.08   | 2328.50 ± 7.50      | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2115.72   |     15.968 |       0.012 | 11.87%  |
| gw_intvl_decode_mean_stat  |       70.3002 |      0.744 |       0.016 | 0.39%   |
| gw_intvl_send_mean_stat    |     2018.13   |     12.028 |       0.009 | 11.32%  |
| lob_intvl_decode_mean_stat |       67.5326 |      0.396 |       0.009 | 0.38%   |
| lob_intvl_apply_mean_stat  |      171.945  |      1.099 |       0.01  | 0.96%   |
| ex2gw_trans_mean_stat      |    11998      |   7715.38  |       0.991 | 67.29%  |
| gw2lob_trans_mean_stat     |     1408.75   |    125.037 |       0.137 | 7.90%   |
| w2w_mean_stat              |    17830.8    |   7422.35  |       0.641 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2139.42   |     18.471 |       0.013 | 10.50%  |
| gw_intvl_decode_mean_stat  |       71.2506 |      0.461 |       0.01  | 0.35%   |
| gw_intvl_send_mean_stat    |     2038.42   |     25.321 |       0.019 | 10.00%  |
| lob_intvl_decode_mean_stat |       67.6995 |      0.326 |       0.007 | 0.33%   |
| lob_intvl_apply_mean_stat  |      200.736  |      1.379 |       0.011 | 0.98%   |
| ex2gw_trans_mean_stat      |    13291.3    |   3014.21  |       0.349 | 65.21%  |
| gw2lob_trans_mean_stat     |     1574.8    |    330.342 |       0.323 | 7.73%   |
| w2w_mean_stat              |    20381.2    |   3372.28  |       0.255 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2201.51   |     48.247 |       0.034 | 7.77%   |
| gw_intvl_decode_mean_stat  |       73.2765 |      0.508 |       0.011 | 0.26%   |
| gw_intvl_send_mean_stat    |     2044.1    |     19.595 |       0.015 | 7.21%   |
| lob_intvl_decode_mean_stat |       69.4492 |      0.368 |       0.008 | 0.25%   |
| lob_intvl_apply_mean_stat  |      180.929  |      2.848 |       0.024 | 0.64%   |
| ex2gw_trans_mean_stat      |    22048.2    |   4435.33  |       0.31  | 77.79%  |
| gw2lob_trans_mean_stat     |     2095      |    394.522 |       0.29  | 7.39%   |
| w2w_mean_stat              |    28343.9    |   4870.69  |       0.265 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |             MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------------:|------------:|
| lob_intvl_apply_p99_stat |   2374.5 |      7   |       0.005 |    21           |       0.015 |
| w2w_p99_stat             | 457669   | 410692   |       1.382 |     1.23208e+06 |       4.146 |
| throughput_stat          | 290018   |   4936.5 |       0.026 | 14809.5         |       0.078 |
### add
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2419 |     11   |       0.007 |     33   |       0.021 |
| w2w_p99_stat             |   488415 | 124682   |       0.393 | 374045   |       1.179 |
| throughput_stat          |   289630 |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2328.5 |     7.5 |       0.005 |     22.5 |       0.015 |
| w2w_p99_stat             | 859723   | 76528.9 |       0.137 | 229587   |       0.411 |
| throughput_stat          | 290176   |  4163.5 |       0.022 |  12490.5 |       0.066 |
