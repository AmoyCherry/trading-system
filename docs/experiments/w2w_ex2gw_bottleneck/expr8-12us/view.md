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
| cross      | 0.27 ± 0.01       | 0.14 ± 0.04   | 2597.50 ± 18.00     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.15 ± 0.04   | 2625.50 ± 37.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.14 ± 0.03   | 2560.50 ± 31.50     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2693.62   |      3.777 |       0.002 | 7.52%   |
| gw_intvl_decode_mean_stat  |       71.6494 |      0.35  |       0.008 | 0.20%   |
| gw_intvl_send_mean_stat    |     2616.27   |     18.071 |       0.011 | 7.31%   |
| lob_intvl_decode_mean_stat |       70.0782 |      0.557 |       0.012 | 0.20%   |
| lob_intvl_apply_mean_stat  |      291.908  |      2.068 |       0.011 | 0.82%   |
| ex2gw_trans_mean_stat      |    14011.3    |   1014.42  |       0.112 | 39.12%  |
| gw2lob_trans_mean_stat     |    16064.1    |    879.21  |       0.084 | 44.85%  |
| w2w_mean_stat              |    35813.9    |   1631.37  |       0.07  | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2721.48   |     19.764 |       0.011 | 7.61%   |
| gw_intvl_decode_mean_stat  |       72.0217 |      0.459 |       0.01  | 0.20%   |
| gw_intvl_send_mean_stat    |     2596.65   |     13.899 |       0.008 | 7.26%   |
| lob_intvl_decode_mean_stat |       69.9951 |      0.323 |       0.007 | 0.20%   |
| lob_intvl_apply_mean_stat  |      322.351  |      1.297 |       0.006 | 0.90%   |
| ex2gw_trans_mean_stat      |    14811.4    |   1318.43  |       0.137 | 41.43%  |
| gw2lob_trans_mean_stat     |    15489.3    |    627.107 |       0.062 | 43.33%  |
| w2w_mean_stat              |    35749      |   1689.4   |       0.073 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2709.1    |     11.825 |       0.007 | 7.57%   |
| gw_intvl_decode_mean_stat  |       74.7173 |      1.166 |       0.024 | 0.21%   |
| gw_intvl_send_mean_stat    |     2614.73   |     33.327 |       0.02  | 7.30%   |
| lob_intvl_decode_mean_stat |       71.3941 |      0.251 |       0.005 | 0.20%   |
| lob_intvl_apply_mean_stat  |      295.706  |      1.805 |       0.009 | 0.83%   |
| ex2gw_trans_mean_stat      |    14465.7    |   1152.96  |       0.123 | 40.41%  |
| gw2lob_trans_mean_stat     |    16324      |    551.258 |       0.052 | 45.60%  |
| w2w_mean_stat              |    35797.2    |   1447.92  |       0.062 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2597.5 |    18   |       0.011 |     54   |       0.033 |
| w2w_p99_stat             | 143635   | 44683.5 |       0.479 | 134050   |       1.437 |
| throughput_stat          | 270496   | 13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |   median |   mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2625.5 |    37 |       0.022 |    111 |       0.066 |
| w2w_p99_stat             | 146104   | 37538 |       0.396 | 112614 |       1.188 |
| throughput_stat          | 273362   | 14446 |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2560.5 |    31.5 |       0.019 |    94.5 |       0.057 |
| w2w_p99_stat             | 144090   | 31019.5 |       0.332 | 93058.5 |       0.996 |
| throughput_stat          | 271992   | 17464.5 |       0.099 | 52393.5 |       0.297 |
