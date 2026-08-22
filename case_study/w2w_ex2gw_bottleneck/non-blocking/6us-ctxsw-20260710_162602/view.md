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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2382.50 ± 9.00      | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.01 ± 0.00   | 2424.50 ± 4.50      | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2352.50 ± 9.00      | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2151.23   |     25.698 |       0.018 | 9.64%   |
| gw_intvl_decode_mean_stat  |       72.3475 |      0.339 |       0.007 | 0.32%   |
| gw_intvl_send_mean_stat    |     2044.32   |     16.772 |       0.013 | 9.16%   |
| lob_intvl_decode_mean_stat |       70.1216 |      0.483 |       0.011 | 0.31%   |
| lob_intvl_apply_mean_stat  |      217.277  |      3.191 |       0.023 | 0.97%   |
| ex2gw_trans_mean_stat      |     8518.24   |    588.997 |       0.107 | 38.17%  |
| gw2lob_trans_mean_stat     |     9069.55   |    302.584 |       0.051 | 40.64%  |
| w2w_mean_stat              |    22318.5    |   1014.89  |       0.07  | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2179.83   |     14.948 |       0.011 | 9.88%   |
| gw_intvl_decode_mean_stat  |       72.1445 |      0.771 |       0.016 | 0.33%   |
| gw_intvl_send_mean_stat    |     2053.37   |     19.796 |       0.015 | 9.30%   |
| lob_intvl_decode_mean_stat |       71.2594 |      1.066 |       0.023 | 0.32%   |
| lob_intvl_apply_mean_stat  |      247.283  |      2.715 |       0.017 | 1.12%   |
| ex2gw_trans_mean_stat      |     8124.14   |    221.326 |       0.042 | 36.81%  |
| gw2lob_trans_mean_stat     |     9284.32   |    489.587 |       0.081 | 42.06%  |
| w2w_mean_stat              |    22072.1    |    706.583 |       0.049 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2169.61   |     17.381 |       0.012 | 10.12%  |
| gw_intvl_decode_mean_stat  |       74.0826 |      0.429 |       0.009 | 0.35%   |
| gw_intvl_send_mean_stat    |     2057.1    |     19.344 |       0.014 | 9.59%   |
| lob_intvl_decode_mean_stat |       71.2142 |      0.805 |       0.017 | 0.33%   |
| lob_intvl_apply_mean_stat  |      223.331  |      2.147 |       0.015 | 1.04%   |
| ex2gw_trans_mean_stat      |     7906.51   |     56.862 |       0.011 | 36.87%  |
| gw2lob_trans_mean_stat     |     8952.23   |    189.342 |       0.033 | 41.75%  |
| w2w_mean_stat              |    21443      |    229.218 |       0.016 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2382.5 |    9   |       0.006 |    27   |       0.018 |
| w2w_p99_stat             |  11702.5 |  223.5 |       0.029 |   670.5 |       0.087 |
| throughput_stat          | 290018   | 4936.5 |       0.026 | 14809.5 |       0.078 |
### add
| metric                   |   median |      mad |   robust cv |       MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------:|------------:|
| lob_intvl_apply_p99_stat |   2424.5 |    4.5   |       0.003 |    13.5   |       0.009 |
| w2w_p99_stat             |  11950   |  325.495 |       0.042 |   976.485 |       0.126 |
| throughput_stat          | 289630   | 4221.5   |       0.022 | 12664.5   |       0.066 |
### cancel
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2352.5 |    9   |       0.006 |    27   |       0.018 |
| w2w_p99_stat             |  11571   |  198.5 |       0.026 |   595.5 |       0.078 |
| throughput_stat          | 290176   | 4163.5 |       0.022 | 12490.5 |       0.066 |
