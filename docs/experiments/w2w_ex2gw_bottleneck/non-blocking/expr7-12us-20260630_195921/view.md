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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2408.00 ± 5.50      | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.01 ± 0.00   | 2468.51 ± 11.00     | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2377.50 ± 7.00      | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2144.32   |     10.071 |       0.007 | 10.98%  |
| gw_intvl_decode_mean_stat  |       70.9127 |      0.279 |       0.006 | 0.36%   |
| gw_intvl_send_mean_stat    |     2003.87   |     30.615 |       0.024 | 10.26%  |
| lob_intvl_decode_mean_stat |       69.2828 |      0.738 |       0.016 | 0.35%   |
| lob_intvl_apply_mean_stat  |      211.3    |      1.464 |       0.011 | 1.08%   |
| ex2gw_trans_mean_stat      |     6536.32   |     38.813 |       0.009 | 33.48%  |
| gw2lob_trans_mean_stat     |     8337.08   |    144.72  |       0.027 | 42.70%  |
| w2w_mean_stat              |    19525.8    |    419.92  |       0.033 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2114.31   |     21.506 |       0.016 | 10.43%  |
| gw_intvl_decode_mean_stat  |       71.1474 |      0.634 |       0.014 | 0.35%   |
| gw_intvl_send_mean_stat    |     2012.78   |     41.511 |       0.032 | 9.93%   |
| lob_intvl_decode_mean_stat |       69.7586 |      0.988 |       0.022 | 0.34%   |
| lob_intvl_apply_mean_stat  |      251.081  |      4.764 |       0.029 | 1.24%   |
| ex2gw_trans_mean_stat      |     6602.43   |     40.422 |       0.009 | 32.58%  |
| gw2lob_trans_mean_stat     |     8989.2    |    649.849 |       0.111 | 44.36%  |
| w2w_mean_stat              |    20262.5    |    670.151 |       0.051 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2123.31   |     22.236 |       0.016 | 10.53%  |
| gw_intvl_decode_mean_stat  |       73.6568 |      1.097 |       0.023 | 0.37%   |
| gw_intvl_send_mean_stat    |     2017.94   |     31.242 |       0.024 | 10.01%  |
| lob_intvl_decode_mean_stat |       70.8694 |      0.651 |       0.014 | 0.35%   |
| lob_intvl_apply_mean_stat  |      218.55   |      1.355 |       0.01  | 1.08%   |
| ex2gw_trans_mean_stat      |     6711.8    |    141.891 |       0.033 | 33.29%  |
| gw2lob_trans_mean_stat     |     8933.72   |    682.416 |       0.118 | 44.32%  |
| w2w_mean_stat              |    20158.8    |    781.488 |       0.06  | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2408 |    5.5   |       0.004 |   16.5   |       0.012 |
| w2w_p99_stat             |    10484 |  104.505 |       0.015 |  313.515 |       0.045 |
| throughput_stat          |   289306 | 3031.5   |       0.016 | 9094.5   |       0.048 |
### add
| metric                   |    median |   mad |   robust cv |   MDE |   min-delta |
|:-------------------------|----------:|------:|------------:|------:|------------:|
| lob_intvl_apply_p99_stat |   2468.51 |    11 |       0.007 |    33 |       0.021 |
| w2w_p99_stat             |  10545.5  |    54 |       0.008 |   162 |       0.024 |
| throughput_stat          | 280929    |  8467 |       0.046 | 25401 |       0.138 |
### cancel
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2377.5 |    7   |       0.005 |   21   |       0.015 |
| w2w_p99_stat             |  10495.5 |   45.5 |       0.007 |  136.5 |       0.021 |
| throughput_stat          | 290380   | 3046   |       0.016 | 9138   |       0.048 |
