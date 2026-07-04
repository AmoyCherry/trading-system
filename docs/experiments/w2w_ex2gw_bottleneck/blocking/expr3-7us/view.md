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
| cross      | 0.27 ± 0.01       | 0.36 ± 0.09   | 2510.00 ± 17.50     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.29 ± 0.08   | 2555.50 ± 17.50     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.85 ± 0.42   | 2466.51 ± 13.00     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2602.48   |     63.123 |       0.037 | 6.79%   |
| gw_intvl_decode_mean_stat  |       73.1096 |      1.782 |       0.038 | 0.19%   |
| gw_intvl_send_mean_stat    |     2474.12   |     41.447 |       0.026 | 6.45%   |
| lob_intvl_decode_mean_stat |       69.5175 |      0.435 |       0.01  | 0.18%   |
| lob_intvl_apply_mean_stat  |      258.45   |      1.061 |       0.006 | 0.67%   |
| ex2gw_trans_mean_stat      |    16824.7    |   1921.02  |       0.176 | 43.87%  |
| gw2lob_trans_mean_stat     |    16980.3    |   1154.53  |       0.105 | 44.28%  |
| w2w_mean_stat              |    38348.3    |   2261.99  |       0.091 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2655.36   |     48.929 |       0.028 | 7.05%   |
| gw_intvl_decode_mean_stat  |       71.6873 |      1.023 |       0.022 | 0.19%   |
| gw_intvl_send_mean_stat    |     2483.41   |     20.616 |       0.013 | 6.59%   |
| lob_intvl_decode_mean_stat |       69.7339 |      0.815 |       0.018 | 0.19%   |
| lob_intvl_apply_mean_stat  |      294.15   |      2.468 |       0.013 | 0.78%   |
| ex2gw_trans_mean_stat      |    15805.2    |   1448.77  |       0.141 | 41.96%  |
| gw2lob_trans_mean_stat     |    16659      |   1066.94  |       0.099 | 44.23%  |
| w2w_mean_stat              |    37664.8    |   1606.74  |       0.066 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2651.79   |     62.139 |       0.036 | 5.37%   |
| gw_intvl_decode_mean_stat  |       74.9411 |      1.789 |       0.037 | 0.15%   |
| gw_intvl_send_mean_stat    |     2522.81   |     57.816 |       0.035 | 5.11%   |
| lob_intvl_decode_mean_stat |       72.2222 |      1.149 |       0.025 | 0.15%   |
| lob_intvl_apply_mean_stat  |      270.292  |      3.867 |       0.022 | 0.55%   |
| ex2gw_trans_mean_stat      |    21354.5    |   4459.48  |       0.322 | 43.24%  |
| gw2lob_trans_mean_stat     |    22641.8    |   3273.82  |       0.223 | 45.85%  |
| w2w_mean_stat              |    49382.3    |   7668.85  |       0.239 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2510 |    17.5 |       0.011 |     52.5 |       0.033 |
| w2w_p99_stat             |   355194 | 89428.4 |       0.388 | 268285   |       1.164 |
| throughput_stat          |   270496 | 13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2555.5 |    17.5 |       0.011 |     52.5 |       0.033 |
| w2w_p99_stat             | 290746   | 75629.1 |       0.401 | 226887   |       1.203 |
| throughput_stat          | 273362   | 14446   |       0.081 |  43338   |       0.243 |
### cancel
| metric                   |    median |      mad |   robust cv |             MDE |   min-delta |
|:-------------------------|----------:|---------:|------------:|----------------:|------------:|
| lob_intvl_apply_p99_stat |   2466.51 |     13   |       0.008 |    39           |       0.024 |
| w2w_p99_stat             | 850205    | 420817   |       0.762 |     1.26245e+06 |       2.286 |
| throughput_stat          | 271992    |  17464.5 |       0.099 | 52393.5         |       0.297 |
