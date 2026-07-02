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
| cross      | 0.29 ± 0.00       | 0.83 ± 0.25   | 2394.50 ± 37.00     | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.49 ± 0.24   | 2498.00 ± 13.50     | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.95 ± 0.10   | 2402.50 ± 17.50     | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2194.95   |     51.669 |       0.036 | 5.82%   |
| gw_intvl_decode_mean_stat  |       71.4102 |      0.776 |       0.017 | 0.19%   |
| gw_intvl_send_mean_stat    |     2005.79   |     21.177 |       0.016 | 5.32%   |
| lob_intvl_decode_mean_stat |       69.8147 |      0.408 |       0.009 | 0.19%   |
| lob_intvl_apply_mean_stat  |      207.621  |      2.287 |       0.017 | 0.55%   |
| ex2gw_trans_mean_stat      |    23925.8    |   7417.36  |       0.478 | 63.48%  |
| gw2lob_trans_mean_stat     |     9542.66   |    207.632 |       0.034 | 25.32%  |
| w2w_mean_stat              |    37692.7    |   6651.32  |       0.272 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2158.62   |     33.37  |       0.024 | 6.08%   |
| gw_intvl_decode_mean_stat  |       71.4609 |      0.812 |       0.017 | 0.20%   |
| gw_intvl_send_mean_stat    |     2000.61   |     14.163 |       0.011 | 5.64%   |
| lob_intvl_decode_mean_stat |       70.1706 |      0.333 |       0.007 | 0.20%   |
| lob_intvl_apply_mean_stat  |      240.589  |      0.705 |       0.005 | 0.68%   |
| ex2gw_trans_mean_stat      |    19761.9    |   4649.55  |       0.362 | 55.66%  |
| gw2lob_trans_mean_stat     |     9893.88   |    481.533 |       0.075 | 27.87%  |
| w2w_mean_stat              |    35502.1    |   3165.39  |       0.137 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2179.95   |     35.34  |       0.025 | 4.61%   |
| gw_intvl_decode_mean_stat  |       72.7052 |      0.362 |       0.008 | 0.15%   |
| gw_intvl_send_mean_stat    |     2027.33   |     17.7   |       0.013 | 4.29%   |
| lob_intvl_decode_mean_stat |       71.7575 |      0.98  |       0.021 | 0.15%   |
| lob_intvl_apply_mean_stat  |      218.083  |      2.375 |       0.017 | 0.46%   |
| ex2gw_trans_mean_stat      |    31952.4    |   8527.29  |       0.411 | 67.57%  |
| gw2lob_trans_mean_stat     |    10019      |    752.736 |       0.116 | 21.19%  |
| w2w_mean_stat              |    47290.9    |   8664.79  |       0.282 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2394.5 |     37   |       0.024 |    111   |       0.072 |
| w2w_p99_stat             | 825595   | 249235   |       0.465 | 747705   |       1.395 |
| throughput_stat          | 289306   |   3031.5 |       0.016 |   9094.5 |       0.048 |
### add
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2498 |     13.5 |       0.008 |     40.5 |       0.024 |
| w2w_p99_stat             |   487862 | 240103   |       0.758 | 720309   |       2.274 |
| throughput_stat          |   280929 |   8467   |       0.046 |  25401   |       0.138 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2402.5 |    17.5 |       0.011 |     52.5 |       0.033 |
| w2w_p99_stat             | 951481   | 96954.6 |       0.157 | 290864   |       0.471 |
| throughput_stat          | 290380   |  3046   |       0.016 |   9138   |       0.048 |
