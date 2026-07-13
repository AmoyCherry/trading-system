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
| cross      | 0.29 ± 0.00       | 0.03 ± 0.02   | 2328.00 ± 8.50      | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.04 ± 0.01   | 2395.00 ± 12.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.02 ± 0.00   | 2314.50 ± 11.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2184.93   |     15.617 |       0.011 | 9.05%   |
| gw_intvl_decode_mean_stat  |       72.3583 |      1.492 |       0.032 | 0.30%   |
| gw_intvl_send_mean_stat    |     2062.41   |     46.463 |       0.035 | 8.54%   |
| lob_intvl_decode_mean_stat |       69.1176 |      0.538 |       0.012 | 0.29%   |
| lob_intvl_apply_mean_stat  |      212.613  |      1.848 |       0.013 | 0.88%   |
| ex2gw_trans_mean_stat      |     9887.12   |   1598.96  |       0.249 | 40.94%  |
| gw2lob_trans_mean_stat     |     9546.25   |    470.613 |       0.076 | 39.53%  |
| w2w_mean_stat              |    24148.3    |   1642.97  |       0.105 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2159.93   |     18.095 |       0.013 | 8.64%   |
| gw_intvl_decode_mean_stat  |       72.6773 |      0.844 |       0.018 | 0.29%   |
| gw_intvl_send_mean_stat    |     2057.6    |     20.43  |       0.015 | 8.23%   |
| lob_intvl_decode_mean_stat |       69.4939 |      0.65  |       0.014 | 0.28%   |
| lob_intvl_apply_mean_stat  |      244.577  |      2.86  |       0.018 | 0.98%   |
| ex2gw_trans_mean_stat      |    10594.6    |   1981.44  |       0.288 | 42.38%  |
| gw2lob_trans_mean_stat     |     9117.47   |    162.81  |       0.028 | 36.47%  |
| w2w_mean_stat              |    24997.8    |   2104.09  |       0.13  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2153.96   |     15.46  |       0.011 | 9.63%   |
| gw_intvl_decode_mean_stat  |       73.5936 |      0.682 |       0.014 | 0.33%   |
| gw_intvl_send_mean_stat    |     2035.58   |     18.73  |       0.014 | 9.10%   |
| lob_intvl_decode_mean_stat |       70.4464 |      0.344 |       0.008 | 0.31%   |
| lob_intvl_apply_mean_stat  |      221.868  |      1.511 |       0.01  | 0.99%   |
| ex2gw_trans_mean_stat      |     8844.76   |    597.36  |       0.104 | 39.53%  |
| gw2lob_trans_mean_stat     |     9032.43   |     38.118 |       0.007 | 40.37%  |
| w2w_mean_stat              |    22375.9    |    704.761 |       0.049 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2328 |     8.5 |       0.006 |    25.5 |       0.018 |
| w2w_p99_stat             |    30649 | 19496.5 |       0.98  | 58489.6 |       2.94  |
| throughput_stat          |   290018 |  4936.5 |       0.026 | 14809.5 |       0.078 |
### add
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2395 |    12.5 |       0.008 |    37.5 |       0.024 |
| w2w_p99_stat             |    38278 | 11898   |       0.479 | 35694   |       1.437 |
| throughput_stat          |   289630 |  4221.5 |       0.022 | 12664.5 |       0.066 |
### cancel
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2314.5 |   11    |       0.007 |    33   |       0.021 |
| w2w_p99_stat             |  17803.5 | 4739.01 |       0.41  | 14217   |       1.23  |
| throughput_stat          | 290176   | 4163.5  |       0.022 | 12490.5 |       0.066 |
