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
| cross      | 0.27 ± 0.01       | 0.14 ± 0.04   | 2549.00 ± 26.50     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.16 ± 0.06   | 2630.50 ± 18.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.20 ± 0.03   | 2560.00 ± 25.00     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2721.61   |     24.533 |       0.014 | 7.80%   |
| gw_intvl_decode_mean_stat  |       72.1641 |      0.841 |       0.018 | 0.21%   |
| gw_intvl_send_mean_stat    |     2653.21   |     22.224 |       0.013 | 7.60%   |
| lob_intvl_decode_mean_stat |       70.1334 |      0.981 |       0.022 | 0.20%   |
| lob_intvl_apply_mean_stat  |      289.735  |      2.686 |       0.014 | 0.83%   |
| ex2gw_trans_mean_stat      |    13582.7    |    420.201 |       0.048 | 38.91%  |
| gw2lob_trans_mean_stat     |    15509      |   1186.21  |       0.118 | 44.42%  |
| w2w_mean_stat              |    34910.9    |   1328.19  |       0.059 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2708.25   |     23.767 |       0.014 | 7.10%   |
| gw_intvl_decode_mean_stat  |       72.0695 |      0.762 |       0.016 | 0.19%   |
| gw_intvl_send_mean_stat    |     2644.44   |     33.954 |       0.02  | 6.94%   |
| lob_intvl_decode_mean_stat |       69.88   |      0.316 |       0.007 | 0.18%   |
| lob_intvl_apply_mean_stat  |      320.652  |      2.146 |       0.01  | 0.84%   |
| ex2gw_trans_mean_stat      |    14618.7    |   2434.71  |       0.257 | 38.35%  |
| gw2lob_trans_mean_stat     |    16191.6    |   1538.28  |       0.146 | 42.47%  |
| w2w_mean_stat              |    38120.9    |   2372.36  |       0.096 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2712.11   |     27.481 |       0.016 | 7.11%   |
| gw_intvl_decode_mean_stat  |       73.1158 |      0.533 |       0.011 | 0.19%   |
| gw_intvl_send_mean_stat    |     2624.56   |     30.361 |       0.018 | 6.88%   |
| lob_intvl_decode_mean_stat |       71.4509 |      0.485 |       0.01  | 0.19%   |
| lob_intvl_apply_mean_stat  |      296.681  |      0.887 |       0.005 | 0.78%   |
| ex2gw_trans_mean_stat      |    15645.9    |   2422.32  |       0.238 | 40.99%  |
| gw2lob_trans_mean_stat     |    17320.9    |    847.154 |       0.075 | 45.38%  |
| w2w_mean_stat              |    38165.6    |   2092.78  |       0.084 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2549 |    26.5 |       0.016 |     79.5 |       0.048 |
| w2w_p99_stat             |   136545 | 43303   |       0.489 | 129909   |       1.467 |
| throughput_stat          |   270496 | 13238.5 |       0.075 |  39715.5 |       0.225 |
### add
| metric                   |   median |     mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2630.5 |    18   |       0.011 |     54 |       0.033 |
| w2w_p99_stat             | 156322   | 57343.1 |       0.565 | 172029 |       1.695 |
| throughput_stat          | 273362   | 14446   |       0.081 |  43338 |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2560 |    25   |       0.015 |    75   |       0.045 |
| w2w_p99_stat             |   195075 | 32653.4 |       0.258 | 97960.2 |       0.774 |
| throughput_stat          |   271992 | 17464.5 |       0.099 | 52393.5 |       0.297 |
