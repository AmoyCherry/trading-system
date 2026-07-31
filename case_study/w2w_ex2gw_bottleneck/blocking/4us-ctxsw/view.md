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
| cross      | 0.29 ± 0.00       | 1.17 ± 0.21   | 2462.00 ± 20.50     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.23 ± 0.12   | 2511.50 ± 13.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 1.38 ± 0.08   | 2427.00 ± 20.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2228.43   |     47.999 |       0.033 | 2.98%   |
| gw_intvl_decode_mean_stat  |       71.3871 |      0.471 |       0.01  | 0.10%   |
| gw_intvl_send_mean_stat    |     2504.06   |     13.439 |       0.008 | 3.35%   |
| lob_intvl_decode_mean_stat |       70.4162 |      0.508 |       0.011 | 0.09%   |
| lob_intvl_apply_mean_stat  |      228.044  |      1.6   |       0.011 | 0.30%   |
| ex2gw_trans_mean_stat      |    54828.2    |  21913.5   |       0.616 | 73.30%  |
| gw2lob_trans_mean_stat     |    14856.1    |    888.48  |       0.092 | 19.86%  |
| w2w_mean_stat              |    74799.8    |  23418.5   |       0.482 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2218.18   |     46.833 |       0.033 | 2.68%   |
| gw_intvl_decode_mean_stat  |       70.9239 |      0.518 |       0.011 | 0.09%   |
| gw_intvl_send_mean_stat    |     2531.95   |     17.888 |       0.011 | 3.06%   |
| lob_intvl_decode_mean_stat |       70.9366 |      1.138 |       0.025 | 0.09%   |
| lob_intvl_apply_mean_stat  |      266.826  |      3.022 |       0.017 | 0.32%   |
| ex2gw_trans_mean_stat      |    59198.6    |  20225.5   |       0.526 | 71.53%  |
| gw2lob_trans_mean_stat     |    16693.4    |   2888.38  |       0.267 | 20.17%  |
| w2w_mean_stat              |    82756.9    |  21784.5   |       0.405 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2256.69   |     74.828 |       0.051 | 1.99%   |
| gw_intvl_decode_mean_stat  |       72.8203 |      0.392 |       0.008 | 0.06%   |
| gw_intvl_send_mean_stat    |     2543.22   |     34.797 |       0.021 | 2.24%   |
| lob_intvl_decode_mean_stat |       72.513  |      1.108 |       0.024 | 0.06%   |
| lob_intvl_apply_mean_stat  |      236.852  |      3.96  |       0.026 | 0.21%   |
| ex2gw_trans_mean_stat      |    88642.7    |  33211.4   |       0.577 | 78.20%  |
| gw2lob_trans_mean_stat     |    17749.1    |   3701.67  |       0.321 | 15.66%  |
| w2w_mean_stat              |   113359      |  33298.7   |       0.452 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2462           |     20.5 |       0.013 |     61.5 |       0.039 |
| w2w_p99_stat             |      1.16515e+06 | 214500   |       0.284 | 643500   |       0.852 |
| throughput_stat          | 290018           |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |          median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2511.5        |     13.5 |       0.008 |     40.5 |       0.024 |
| w2w_p99_stat             |      1.2348e+06 | 120398   |       0.15  | 361193   |       0.45  |
| throughput_stat          | 289630          |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2427           |    20   |       0.013 |     60   |       0.039 |
| w2w_p99_stat             |      1.38453e+06 | 80545.6 |       0.09  | 241637   |       0.27  |
| throughput_stat          | 290176           |  4163.5 |       0.022 |  12490.5 |       0.066 |
