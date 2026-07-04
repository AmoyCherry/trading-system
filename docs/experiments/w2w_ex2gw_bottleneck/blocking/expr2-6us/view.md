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
| cross      | 0.27 ± 0.01       | 0.39 ± 0.15   | 2490.00 ± 10.00     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.23 ± 0.12   | 2563.50 ± 9.00      | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.31 ± 0.07   | 2457.50 ± 13.00     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2540.1    |     26.183 |       0.016 | 6.54%   |
| gw_intvl_decode_mean_stat  |       72.6233 |      0.9   |       0.019 | 0.19%   |
| gw_intvl_send_mean_stat    |     2429.27   |     34.195 |       0.022 | 6.26%   |
| lob_intvl_decode_mean_stat |       69.9036 |      0.545 |       0.012 | 0.18%   |
| lob_intvl_apply_mean_stat  |      255.54   |      1.918 |       0.012 | 0.66%   |
| ex2gw_trans_mean_stat      |    17653.5    |   2063.47  |       0.18  | 45.47%  |
| gw2lob_trans_mean_stat     |    16936      |   1131.23  |       0.103 | 43.63%  |
| w2w_mean_stat              |    38821.6    |   2595.13  |       0.103 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2517.97   |     29.728 |       0.018 | 7.21%   |
| gw_intvl_decode_mean_stat  |       71.2542 |      0.456 |       0.01  | 0.20%   |
| gw_intvl_send_mean_stat    |     2380.95   |     18.894 |       0.012 | 6.82%   |
| lob_intvl_decode_mean_stat |       69.3675 |      0.389 |       0.009 | 0.20%   |
| lob_intvl_apply_mean_stat  |      289.05   |      1.827 |       0.01  | 0.83%   |
| ex2gw_trans_mean_stat      |    13312.5    |   2363.9   |       0.274 | 38.13%  |
| gw2lob_trans_mean_stat     |    16231.4    |   1281.39  |       0.122 | 46.50%  |
| w2w_mean_stat              |    34909.5    |   4764.1   |       0.21  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2508.59   |     14.548 |       0.009 | 6.88%   |
| gw_intvl_decode_mean_stat  |       73.8011 |      0.889 |       0.019 | 0.20%   |
| gw_intvl_send_mean_stat    |     2425.81   |     32.205 |       0.02  | 6.65%   |
| lob_intvl_decode_mean_stat |       71.2348 |      0.589 |       0.013 | 0.20%   |
| lob_intvl_apply_mean_stat  |      261.295  |      1.705 |       0.01  | 0.72%   |
| ex2gw_trans_mean_stat      |    15173.2    |   1381.26  |       0.14  | 41.59%  |
| gw2lob_trans_mean_stat     |    15818.6    |   1335.79  |       0.13  | 43.36%  |
| w2w_mean_stat              |    36482.3    |   2253.86  |       0.095 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2490 |     10   |       0.006 |     30   |       0.018 |
| w2w_p99_stat             |   388981 | 148043   |       0.586 | 444128   |       1.758 |
| throughput_stat          |   270496 |  13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2563.5 |      9 |       0.005 |     27 |       0.015 |
| w2w_p99_stat             | 225177   | 123484 |       0.845 | 370453 |       2.535 |
| throughput_stat          | 273362   |  14446 |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2457.5 |    13   |       0.008 |     39   |       0.024 |
| w2w_p99_stat             | 307249   | 65085   |       0.326 | 195255   |       0.978 |
| throughput_stat          | 271992   | 17464.5 |       0.099 |  52393.5 |       0.297 |
