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
| cross      | 0.29 ± 0.00       | 1.30 ± 0.18   | 2460.50 ± 64.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.40 ± 0.17   | 2577.00 ± 36.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 1.23 ± 0.15   | 2504.00 ± 53.50     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3903.43   |     68.57  |       0.027 | 0.59%   |
| gw_intvl_decode_mean_stat  |       71.8806 |      1.107 |       0.024 | 0.01%   |
| gw_intvl_send_mean_stat    |     2510.53   |     53.916 |       0.033 | 0.38%   |
| lob_intvl_decode_mean_stat |       70.6552 |      0.348 |       0.008 | 0.01%   |
| lob_intvl_apply_mean_stat  |      229.763  |      1.831 |       0.012 | 0.03%   |
| ex2gw_trans_mean_stat      |   640181      |  17274.6   |       0.042 | 96.35%  |
| gw2lob_trans_mean_stat     |    14777.7    |   2759.57  |       0.288 | 2.22%   |
| w2w_mean_stat              |   664440      |  20843.1   |       0.048 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3943.84   |    116.879 |       0.046 | 0.59%   |
| gw_intvl_decode_mean_stat  |       72.0941 |      1.906 |       0.041 | 0.01%   |
| gw_intvl_send_mean_stat    |     2509.53   |     57.31  |       0.035 | 0.38%   |
| lob_intvl_decode_mean_stat |       70.5552 |      0.53  |       0.012 | 0.01%   |
| lob_intvl_apply_mean_stat  |      269.734  |      3.858 |       0.022 | 0.04%   |
| ex2gw_trans_mean_stat      |   637522      |  22481.9   |       0.054 | 95.59%  |
| gw2lob_trans_mean_stat     |    18114.9    |   2789.98  |       0.237 | 2.72%   |
| w2w_mean_stat              |   666944      |  24510.3   |       0.057 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3834.53   |     79.215 |       0.032 | 0.59%   |
| gw_intvl_decode_mean_stat  |       72.6148 |      1.359 |       0.029 | 0.01%   |
| gw_intvl_send_mean_stat    |     2465.62   |     34.325 |       0.021 | 0.38%   |
| lob_intvl_decode_mean_stat |       72.5895 |      1.085 |       0.023 | 0.01%   |
| lob_intvl_apply_mean_stat  |      239.547  |      3.178 |       0.02  | 0.04%   |
| ex2gw_trans_mean_stat      |   628868      |  15174.8   |       0.037 | 96.49%  |
| gw2lob_trans_mean_stat     |    17469.7    |   3170.77  |       0.28  | 2.68%   |
| w2w_mean_stat              |   651774      |  19390.6   |       0.046 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2460.5         |     64   |       0.04  |    192   |       0.12  |
| w2w_p99_stat             |      1.30299e+06 | 178274   |       0.211 | 534823   |       0.633 |
| throughput_stat          | 290018           |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2577           |     36.5 |       0.022 |    109.5 |       0.066 |
| w2w_p99_stat             |      1.40498e+06 | 171151   |       0.188 | 513454   |       0.564 |
| throughput_stat          | 289630           |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2504           |     53.5 |       0.033 |    160.5 |       0.099 |
| w2w_p99_stat             |      1.22556e+06 | 149106   |       0.187 | 447317   |       0.561 |
| throughput_stat          | 290176           |   4163.5 |       0.022 |  12490.5 |       0.066 |
