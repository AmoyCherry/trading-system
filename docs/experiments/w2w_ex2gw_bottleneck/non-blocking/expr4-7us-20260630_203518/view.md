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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2511.50 ± 7.51      | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.01 ± 0.00   | 2572.00 ± 8.00      | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2496.00 ± 2.00      | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2156.12   |     16.123 |       0.012 | 9.61%   |
| gw_intvl_decode_mean_stat  |       71.2253 |      0.876 |       0.019 | 0.32%   |
| gw_intvl_send_mean_stat    |     2003.66   |     16.677 |       0.013 | 8.93%   |
| lob_intvl_decode_mean_stat |       69.9851 |      0.477 |       0.011 | 0.31%   |
| lob_intvl_apply_mean_stat  |      213.909  |      2.213 |       0.016 | 0.95%   |
| ex2gw_trans_mean_stat      |     8375.76   |    399.069 |       0.073 | 37.34%  |
| gw2lob_trans_mean_stat     |     9415.41   |    251.221 |       0.041 | 41.98%  |
| w2w_mean_stat              |    22430.1    |    828.946 |       0.057 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2147.52   |     34.622 |       0.025 | 9.60%   |
| gw_intvl_decode_mean_stat  |       70.9023 |      0.369 |       0.008 | 0.32%   |
| gw_intvl_send_mean_stat    |     2020.97   |     15.733 |       0.012 | 9.04%   |
| lob_intvl_decode_mean_stat |       69.9248 |      0.542 |       0.012 | 0.31%   |
| lob_intvl_apply_mean_stat  |      249.249  |      2.76  |       0.017 | 1.11%   |
| ex2gw_trans_mean_stat      |     8114.35   |    272.471 |       0.052 | 36.28%  |
| gw2lob_trans_mean_stat     |     9718.42   |    490.058 |       0.078 | 43.45%  |
| w2w_mean_stat              |    22365      |    719.626 |       0.05  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2133.88   |     12.856 |       0.009 | 9.41%   |
| gw_intvl_decode_mean_stat  |       72.4993 |      0.68  |       0.014 | 0.32%   |
| gw_intvl_send_mean_stat    |     2024.22   |     21.686 |       0.017 | 8.92%   |
| lob_intvl_decode_mean_stat |       71.7566 |      0.633 |       0.014 | 0.32%   |
| lob_intvl_apply_mean_stat  |      223.012  |      1.984 |       0.014 | 0.98%   |
| ex2gw_trans_mean_stat      |     8510.11   |    433.64  |       0.078 | 37.51%  |
| gw2lob_trans_mean_stat     |     9583.28   |    475.243 |       0.076 | 42.24%  |
| w2w_mean_stat              |    22686      |    972.911 |       0.066 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2511.5 |    7.505 |       0.005 |   22.515 |       0.015 |
| w2w_p99_stat             |  10845.5 |  318     |       0.045 |  954     |       0.135 |
| throughput_stat          | 289306   | 3031.5   |       0.016 | 9094.5   |       0.048 |
### add
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2572 |    8   |       0.005 |    24   |       0.015 |
| w2w_p99_stat             |    11176 |  388.5 |       0.054 |  1165.5 |       0.162 |
| throughput_stat          |   280929 | 8467   |       0.046 | 25401   |       0.138 |
### cancel
| metric                   |   median |   mad |   robust cv |   MDE |   min-delta |
|:-------------------------|---------:|------:|------------:|------:|------------:|
| lob_intvl_apply_p99_stat |     2496 |     2 |       0.001 |     6 |       0.003 |
| w2w_p99_stat             |    11095 |   282 |       0.039 |   846 |       0.117 |
| throughput_stat          |   290380 |  3046 |       0.016 |  9138 |       0.048 |
