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
| cross      | 0.27 ± 0.01       | 0.58 ± 0.15   | 2530.00 ± 27.50     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.30 ± 0.10   | 2597.50 ± 17.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.26 ± 0.05   | 2476.00 ± 20.50     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2785.02   |     21.45  |       0.012 | 6.07%   |
| gw_intvl_decode_mean_stat  |       71.9211 |      1.229 |       0.026 | 0.16%   |
| gw_intvl_send_mean_stat    |     2665.7    |     25.702 |       0.015 | 5.81%   |
| lob_intvl_decode_mean_stat |       70.7047 |      0.822 |       0.018 | 0.15%   |
| lob_intvl_apply_mean_stat  |      288.661  |      2.113 |       0.011 | 0.63%   |
| ex2gw_trans_mean_stat      |    19834.5    |   2820.48  |       0.219 | 43.21%  |
| gw2lob_trans_mean_stat     |    20467.9    |   1784.09  |       0.134 | 44.59%  |
| w2w_mean_stat              |    45899.6    |   4702.36  |       0.158 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2762.62   |     26.731 |       0.015 | 6.93%   |
| gw_intvl_decode_mean_stat  |       71.3841 |      0.914 |       0.02  | 0.18%   |
| gw_intvl_send_mean_stat    |     2638.14   |     20.71  |       0.012 | 6.62%   |
| lob_intvl_decode_mean_stat |       70.9015 |      0.172 |       0.004 | 0.18%   |
| lob_intvl_apply_mean_stat  |      321.25   |      1.566 |       0.008 | 0.81%   |
| ex2gw_trans_mean_stat      |    16463      |    410.507 |       0.038 | 41.31%  |
| gw2lob_trans_mean_stat     |    17881.1    |   1573.94  |       0.136 | 44.87%  |
| w2w_mean_stat              |    39852      |   1316.24  |       0.051 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2785.08   |     11.275 |       0.006 | 7.16%   |
| gw_intvl_decode_mean_stat  |       74.4195 |      1.823 |       0.038 | 0.19%   |
| gw_intvl_send_mean_stat    |     2673.71   |     15.161 |       0.009 | 6.88%   |
| lob_intvl_decode_mean_stat |       71.809  |      0.503 |       0.011 | 0.18%   |
| lob_intvl_apply_mean_stat  |      293.727  |      1.322 |       0.007 | 0.76%   |
| ex2gw_trans_mean_stat      |    16651.2    |    783.218 |       0.072 | 42.82%  |
| gw2lob_trans_mean_stat     |    17211.2    |   1317.67  |       0.118 | 44.26%  |
| w2w_mean_stat              |    38889.1    |   1853.26  |       0.073 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2530 |     27.5 |       0.017 |     82.5 |       0.051 |
| w2w_p99_stat             |   583227 | 149906   |       0.396 | 449719   |       1.188 |
| throughput_stat          |   270496 |  13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2597.5 |     17 |       0.01  |     51 |       0.03  |
| w2w_p99_stat             | 304833   | 100460 |       0.508 | 301381 |       1.524 |
| throughput_stat          | 273362   |  14446 |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2476 |    20.5 |       0.013 |     61.5 |       0.039 |
| w2w_p99_stat             |   263144 | 52658.9 |       0.308 | 157977   |       0.924 |
| throughput_stat          |   271992 | 17464.5 |       0.099 |  52393.5 |       0.297 |
