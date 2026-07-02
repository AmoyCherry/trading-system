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
| cross      | 0.29 ± 0.00       | 1.41 ± 0.11   | 2410.50 ± 15.00     | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 1.28 ± 0.17   | 2474.00 ± 10.50     | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.88 ± 0.51   | 2390.00 ± 20.00     | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2435.64   |     50.369 |       0.032 | 3.23%   |
| gw_intvl_decode_mean_stat  |       72.8269 |      1.944 |       0.041 | 0.10%   |
| gw_intvl_send_mean_stat    |     2578.05   |     51.651 |       0.031 | 3.42%   |
| lob_intvl_decode_mean_stat |       70.993  |      0.895 |       0.019 | 0.09%   |
| lob_intvl_apply_mean_stat  |      247.766  |      3.552 |       0.022 | 0.33%   |
| ex2gw_trans_mean_stat      |    48675.4    |  26861.9   |       0.85  | 64.51%  |
| gw2lob_trans_mean_stat     |    20944.9    |   3410.15  |       0.251 | 27.76%  |
| w2w_mean_stat              |    75452.3    |  31220.4   |       0.637 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2408.79   |     44.12  |       0.028 | 3.83%   |
| gw_intvl_decode_mean_stat  |       73.0565 |      1.912 |       0.04  | 0.12%   |
| gw_intvl_send_mean_stat    |     2573.93   |     40.239 |       0.024 | 4.10%   |
| lob_intvl_decode_mean_stat |       70.5916 |      0.724 |       0.016 | 0.11%   |
| lob_intvl_apply_mean_stat  |      283.975  |      4.049 |       0.022 | 0.45%   |
| ex2gw_trans_mean_stat      |    40212.4    |  17183.7   |       0.658 | 64.01%  |
| gw2lob_trans_mean_stat     |    18427.5    |   2229.05  |       0.186 | 29.33%  |
| w2w_mean_stat              |    62824.2    |  17093.2   |       0.419 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2381.06   |     32.228 |       0.021 | 4.97%   |
| gw_intvl_decode_mean_stat  |       73.3442 |      1.449 |       0.03  | 0.15%   |
| gw_intvl_send_mean_stat    |     2537.19   |     18.874 |       0.011 | 5.29%   |
| lob_intvl_decode_mean_stat |       72.1499 |      0.525 |       0.011 | 0.15%   |
| lob_intvl_apply_mean_stat  |      252.915  |      3.427 |       0.021 | 0.53%   |
| ex2gw_trans_mean_stat      |    26941.6    |  12451.9   |       0.712 | 56.18%  |
| gw2lob_trans_mean_stat     |    16831.1    |   2355.09  |       0.216 | 35.10%  |
| w2w_mean_stat              |    47954.6    |  12539.5   |       0.403 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2410.5         |     15   |       0.01  |     45   |       0.03  |
| w2w_p99_stat             |      1.41119e+06 | 106307   |       0.116 | 318921   |       0.348 |
| throughput_stat          | 289306           |   3031.5 |       0.016 |   9094.5 |       0.048 |
### add
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2474           |     10.5 |       0.007 |     31.5 |       0.021 |
| w2w_p99_stat             |      1.27731e+06 | 167752   |       0.202 | 503257   |       0.606 |
| throughput_stat          | 280929           |   8467   |       0.046 |  25401   |       0.138 |
### cancel
| metric                   |   median |    mad |   robust cv |            MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|---------------:|------------:|
| lob_intvl_apply_p99_stat |     2390 |     20 |       0.013 |   60           |       0.039 |
| w2w_p99_stat             |   877904 | 508386 |       0.892 |    1.52516e+06 |       2.676 |
| throughput_stat          |   290380 |   3046 |       0.016 | 9138           |       0.048 |
