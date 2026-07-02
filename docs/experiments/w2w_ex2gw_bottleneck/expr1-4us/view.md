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
| cross      | 0.27 ± 0.01       | 1.28 ± 0.15   | 2379.00 ± 18.50     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 1.24 ± 0.17   | 2440.00 ± 11.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 1.04 ± 0.19   | 2362.50 ± 13.50     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2233.95   |     62.692 |       0.043 | 2.61%   |
| gw_intvl_decode_mean_stat  |       71.2412 |      0.769 |       0.017 | 0.08%   |
| gw_intvl_send_mean_stat    |     2530.38   |     25.196 |       0.015 | 2.96%   |
| lob_intvl_decode_mean_stat |       69.7325 |      0.802 |       0.018 | 0.08%   |
| lob_intvl_apply_mean_stat  |      228.124  |      2.926 |       0.02  | 0.27%   |
| ex2gw_trans_mean_stat      |    60357.4    |  26705.7   |       0.682 | 70.61%  |
| gw2lob_trans_mean_stat     |    15624.9    |   2549.8   |       0.251 | 18.28%  |
| w2w_mean_stat              |    85477.1    |  31812.1   |       0.573 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2194.25   |     39.734 |       0.028 | 2.95%   |
| gw_intvl_decode_mean_stat  |       70.8482 |      0.833 |       0.018 | 0.10%   |
| gw_intvl_send_mean_stat    |     2553.66   |     27.035 |       0.016 | 3.43%   |
| lob_intvl_decode_mean_stat |       69.7156 |      0.32  |       0.007 | 0.09%   |
| lob_intvl_apply_mean_stat  |      267.357  |      3.736 |       0.022 | 0.36%   |
| ex2gw_trans_mean_stat      |    53850.2    |  15148.9   |       0.433 | 72.29%  |
| gw2lob_trans_mean_stat     |    15009.1    |   2478.62  |       0.254 | 20.15%  |
| w2w_mean_stat              |    74489.1    |  17386.2   |       0.36  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2172.83   |     16.937 |       0.012 | 3.22%   |
| gw_intvl_decode_mean_stat  |       72.0801 |      0.617 |       0.013 | 0.11%   |
| gw_intvl_send_mean_stat    |     2522.94   |      6.894 |       0.004 | 3.74%   |
| lob_intvl_decode_mean_stat |       70.5363 |      0.798 |       0.017 | 0.10%   |
| lob_intvl_apply_mean_stat  |      235.992  |      2.122 |       0.014 | 0.35%   |
| ex2gw_trans_mean_stat      |    44743.6    |  12706.9   |       0.437 | 66.29%  |
| gw2lob_trans_mean_stat     |    15011.4    |   1727.93  |       0.177 | 22.24%  |
| w2w_mean_stat              |    67501.2    |  13692     |       0.312 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2379           |     18.5 |       0.012 |     55.5 |       0.036 |
| w2w_p99_stat             |      1.28492e+06 | 152357   |       0.183 | 457071   |       0.549 |
| throughput_stat          | 270496           |  13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |           median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|-----------------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2440           |     11 |       0.007 |     33 |       0.021 |
| w2w_p99_stat             |      1.23772e+06 | 170468 |       0.212 | 511404 |       0.636 |
| throughput_stat          | 273362           |  14446 |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2362.5         |     13.5 |       0.009 |     40.5 |       0.027 |
| w2w_p99_stat             |      1.03727e+06 | 186476   |       0.277 | 559429   |       0.831 |
| throughput_stat          | 271992           |  17464.5 |       0.099 |  52393.5 |       0.297 |
