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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2478.50 ± 15.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.01 ± 0.00   | 2509.00 ± 7.00      | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2438.50 ± 28.50     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2148.99   |     16.998 |       0.012 | 10.91%  |
| gw_intvl_decode_mean_stat  |       73.0272 |      0.702 |       0.015 | 0.37%   |
| gw_intvl_send_mean_stat    |     2045.77   |     34.325 |       0.026 | 10.38%  |
| lob_intvl_decode_mean_stat |       70.3294 |      0.866 |       0.019 | 0.36%   |
| lob_intvl_apply_mean_stat  |      221.391  |      5.04  |       0.035 | 1.12%   |
| ex2gw_trans_mean_stat      |     6663.15   |    138.511 |       0.032 | 33.81%  |
| gw2lob_trans_mean_stat     |     8359.78   |    171.777 |       0.032 | 42.42%  |
| w2w_mean_stat              |    19705.6    |    484.805 |       0.038 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2171.1    |     18.145 |       0.013 | 10.98%  |
| gw_intvl_decode_mean_stat  |       73.0798 |      0.552 |       0.012 | 0.37%   |
| gw_intvl_send_mean_stat    |     2055.13   |     48.998 |       0.037 | 10.39%  |
| lob_intvl_decode_mean_stat |       69.4563 |      0.146 |       0.003 | 0.35%   |
| lob_intvl_apply_mean_stat  |      250.737  |      2.171 |       0.013 | 1.27%   |
| ex2gw_trans_mean_stat      |     6627.78   |    103.649 |       0.024 | 33.51%  |
| gw2lob_trans_mean_stat     |     8410.03   |    204.404 |       0.037 | 42.52%  |
| w2w_mean_stat              |    19779.3    |    430.676 |       0.034 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2144.99   |      8.912 |       0.006 | 10.93%  |
| gw_intvl_decode_mean_stat  |       74.8888 |      0.473 |       0.01  | 0.38%   |
| gw_intvl_send_mean_stat    |     2011.2    |      6.514 |       0.005 | 10.25%  |
| lob_intvl_decode_mean_stat |       71.6279 |      0.768 |       0.017 | 0.36%   |
| lob_intvl_apply_mean_stat  |      229.809  |      1.743 |       0.012 | 1.17%   |
| ex2gw_trans_mean_stat      |     6625.09   |    101.922 |       0.024 | 33.75%  |
| gw2lob_trans_mean_stat     |     8472.88   |    264.115 |       0.048 | 43.16%  |
| w2w_mean_stat              |    19630.6    |    341.259 |       0.027 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2478.5 |   15   |       0.009 |    45   |       0.027 |
| w2w_p99_stat             |  10800.5 |  127.5 |       0.018 |   382.5 |       0.054 |
| throughput_stat          | 290018   | 4936.5 |       0.026 | 14809.5 |       0.078 |
### add
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2509 |    7   |       0.004 |    21   |       0.012 |
| w2w_p99_stat             |    10841 |   86.5 |       0.012 |   259.5 |       0.036 |
| throughput_stat          |   289630 | 4221.5 |       0.022 | 12664.5 |       0.066 |
### cancel
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2438.5 |   28.5 |       0.018 |    85.5 |       0.054 |
| w2w_p99_stat             |  10744   |  159.5 |       0.023 |   478.5 |       0.069 |
| throughput_stat          | 290176   | 4163.5 |       0.022 | 12490.5 |       0.066 |
