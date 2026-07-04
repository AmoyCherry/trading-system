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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2432.50 ± 6.00      | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.01 ± 0.00   | 2483.50 ± 5.50      | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2391.51 ± 14.01     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2131.66   |      8.441 |       0.006 | 10.45%  |
| gw_intvl_decode_mean_stat  |       72.2326 |      0.433 |       0.009 | 0.35%   |
| gw_intvl_send_mean_stat    |     2134.73   |     21.988 |       0.016 | 10.47%  |
| lob_intvl_decode_mean_stat |       67.3605 |      0.363 |       0.008 | 0.33%   |
| lob_intvl_apply_mean_stat  |      217.819  |      2.088 |       0.015 | 1.07%   |
| ex2gw_trans_mean_stat      |     6786.95   |    150.58  |       0.034 | 33.28%  |
| gw2lob_trans_mean_stat     |     8699.78   |    467.222 |       0.083 | 42.66%  |
| w2w_mean_stat              |    20395.3    |    850.741 |       0.064 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2150.16   |     11.566 |       0.008 | 10.47%  |
| gw_intvl_decode_mean_stat  |       73.4025 |      1.184 |       0.025 | 0.36%   |
| gw_intvl_send_mean_stat    |     2020.76   |     16.04  |       0.012 | 9.84%   |
| lob_intvl_decode_mean_stat |       67.5175 |      0.166 |       0.004 | 0.33%   |
| lob_intvl_apply_mean_stat  |      251.367  |      1.178 |       0.007 | 1.22%   |
| ex2gw_trans_mean_stat      |     6583.46   |     88.9   |       0.021 | 32.07%  |
| gw2lob_trans_mean_stat     |     8770.56   |    562.185 |       0.099 | 42.72%  |
| w2w_mean_stat              |    20529.6    |    722.631 |       0.054 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2139.04   |      7.821 |       0.006 | 11.00%  |
| gw_intvl_decode_mean_stat  |       74.0683 |      0.472 |       0.01  | 0.38%   |
| gw_intvl_send_mean_stat    |     2015.63   |     12.57  |       0.01  | 10.37%  |
| lob_intvl_decode_mean_stat |       68.4514 |      0.238 |       0.005 | 0.35%   |
| lob_intvl_apply_mean_stat  |      223.078  |      1.974 |       0.014 | 1.15%   |
| ex2gw_trans_mean_stat      |     6572.76   |     68.178 |       0.016 | 33.80%  |
| gw2lob_trans_mean_stat     |     8382.19   |    137.083 |       0.025 | 43.11%  |
| w2w_mean_stat              |    19443.6    |    193.882 |       0.015 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |       MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------:|------------:|
| lob_intvl_apply_p99_stat |   2432.5 |    6     |       0.004 |    18     |       0.012 |
| w2w_p99_stat             |  10687.5 |  108.995 |       0.016 |   326.985 |       0.048 |
| throughput_stat          | 290018   | 4936.5   |       0.026 | 14809.5   |       0.078 |
### add
| metric                   |   median |      mad |   robust cv |       MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------:|------------:|
| lob_intvl_apply_p99_stat |   2483.5 |    5.5   |       0.003 |    16.5   |       0.009 |
| w2w_p99_stat             |  10764.5 |  118.995 |       0.017 |   356.985 |       0.051 |
| throughput_stat          | 289630   | 4221.5   |       0.022 | 12664.5   |       0.066 |
### cancel
| metric                   |    median |      mad |   robust cv |       MDE |   min-delta |
|:-------------------------|----------:|---------:|------------:|----------:|------------:|
| lob_intvl_apply_p99_stat |   2391.51 |   14.005 |       0.009 |    42.015 |       0.027 |
| w2w_p99_stat             |  10578.5  |  109.5   |       0.016 |   328.5   |       0.048 |
| throughput_stat          | 290176    | 4163.5   |       0.022 | 12490.5   |       0.066 |
