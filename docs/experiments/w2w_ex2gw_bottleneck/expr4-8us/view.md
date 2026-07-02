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
| cross      | 0.27 ± 0.01       | 0.73 ± 0.36   | 2517.00 ± 31.00     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.33 ± 0.16   | 2577.00 ± 35.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.20 ± 0.05   | 2489.50 ± 11.00     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2864.38   |     14.651 |       0.008 | 6.15%   |
| gw_intvl_decode_mean_stat  |       73.8087 |      2.341 |       0.049 | 0.16%   |
| gw_intvl_send_mean_stat    |     2580.82   |     24.871 |       0.015 | 5.55%   |
| lob_intvl_decode_mean_stat |       70.136  |      0.588 |       0.013 | 0.15%   |
| lob_intvl_apply_mean_stat  |      272.92   |      3.096 |       0.017 | 0.59%   |
| ex2gw_trans_mean_stat      |    19736.4    |   4474.28  |       0.349 | 42.41%  |
| gw2lob_trans_mean_stat     |    20656.5    |   3598.1   |       0.268 | 44.38%  |
| w2w_mean_stat              |    46541.9    |   7868.21  |       0.26  | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2861.21   |     29.437 |       0.016 | 7.35%   |
| gw_intvl_decode_mean_stat  |       72.0182 |      0.7   |       0.015 | 0.19%   |
| gw_intvl_send_mean_stat    |     2541.08   |     30.06  |       0.018 | 6.53%   |
| lob_intvl_decode_mean_stat |       69.445  |      0.424 |       0.009 | 0.18%   |
| lob_intvl_apply_mean_stat  |      303.705  |      1.459 |       0.007 | 0.78%   |
| ex2gw_trans_mean_stat      |    14864.2    |   1030.89  |       0.107 | 38.19%  |
| gw2lob_trans_mean_stat     |    18067.6    |   1217.41  |       0.104 | 46.41%  |
| w2w_mean_stat              |    38926.4    |   1896.79  |       0.075 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2847.3    |     26.758 |       0.014 | 7.82%   |
| gw_intvl_decode_mean_stat  |       73.7481 |      1.184 |       0.025 | 0.20%   |
| gw_intvl_send_mean_stat    |     2493.45   |     20.642 |       0.013 | 6.85%   |
| lob_intvl_decode_mean_stat |       71.144  |      0.201 |       0.004 | 0.20%   |
| lob_intvl_apply_mean_stat  |      277.714  |      0.268 |       0.001 | 0.76%   |
| ex2gw_trans_mean_stat      |    15190.2    |   1071.95  |       0.109 | 41.71%  |
| gw2lob_trans_mean_stat     |    16167.4    |    433.779 |       0.041 | 44.39%  |
| w2w_mean_stat              |    36419.1    |   1017.13  |       0.043 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |             MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------------:|------------:|
| lob_intvl_apply_p99_stat |     2517 |     31   |       0.019 |    93           |       0.057 |
| w2w_p99_stat             |   730612 | 360259   |       0.76  |     1.08078e+06 |       2.28  |
| throughput_stat          |   270496 |  13238.5 |       0.075 | 39715.5         |       0.225 |
### add
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |     2577 |     35 |       0.021 |    105 |       0.063 |
| w2w_p99_stat             |   330632 | 164344 |       0.766 | 493032 |       2.298 |
| throughput_stat          |   273362 |  14446 |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2489.5 |    11   |       0.007 |     33   |       0.021 |
| w2w_p99_stat             | 202693   | 45492   |       0.346 | 136476   |       1.038 |
| throughput_stat          | 271992   | 17464.5 |       0.099 |  52393.5 |       0.297 |
