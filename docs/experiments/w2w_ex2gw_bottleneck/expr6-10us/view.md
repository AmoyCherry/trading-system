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
| cross      | 0.27 ± 0.01       | 0.28 ± 0.10   | 2550.50 ± 24.51     | 437.02, [266.61, 592.61]     | 58.70, [-97.89, 187.39]     | 2992.33 ± 73.54 |
| add        | 0.27 ± 0.01       | 0.25 ± 0.03   | 2573.00 ± 11.00     | 664.31, [522.49, 754.64]     | 61.90, [7.40, 166.64]       | 2925.36 ± 34.02 |
| cancel     | 0.27 ± 0.02       | 0.20 ± 0.06   | 2506.50 ± 31.00     | 472.41, [299.53, 648.51]     | 32.25, [-106.81, 209.10]    | 2996.36 ± 70.82 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2741.51   |     25.457 |       0.014 | 6.96%   |
| gw_intvl_decode_mean_stat  |       72.0971 |      0.501 |       0.011 | 0.18%   |
| gw_intvl_send_mean_stat    |     2600.61   |     31.416 |       0.019 | 6.60%   |
| lob_intvl_decode_mean_stat |       70.5314 |      0.4   |       0.009 | 0.18%   |
| lob_intvl_apply_mean_stat  |      287.292  |      1.432 |       0.008 | 0.73%   |
| ex2gw_trans_mean_stat      |    16232.2    |    856.454 |       0.081 | 41.19%  |
| gw2lob_trans_mean_stat     |    17966.6    |   1578.77  |       0.135 | 45.59%  |
| w2w_mean_stat              |    39410.4    |   2423.61  |       0.095 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2743.5    |     18.953 |       0.011 | 7.26%   |
| gw_intvl_decode_mean_stat  |       71.7598 |      0.218 |       0.005 | 0.19%   |
| gw_intvl_send_mean_stat    |     2601.22   |      9.054 |       0.005 | 6.89%   |
| lob_intvl_decode_mean_stat |       70.5816 |      0.315 |       0.007 | 0.19%   |
| lob_intvl_apply_mean_stat  |      321.078  |      2.14  |       0.01  | 0.85%   |
| ex2gw_trans_mean_stat      |    15635.9    |   1412.87  |       0.139 | 41.40%  |
| gw2lob_trans_mean_stat     |    17339.4    |   1734.86  |       0.154 | 45.91%  |
| w2w_mean_stat              |    37769.6    |   2639.01  |       0.108 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2764.23   |     41.706 |       0.023 | 6.98%   |
| gw_intvl_decode_mean_stat  |       73.2099 |      0.491 |       0.01  | 0.18%   |
| gw_intvl_send_mean_stat    |     2642.91   |     51.9   |       0.03  | 6.67%   |
| lob_intvl_decode_mean_stat |       71.5724 |      0.507 |       0.011 | 0.18%   |
| lob_intvl_apply_mean_stat  |      293.51   |      1.786 |       0.009 | 0.74%   |
| ex2gw_trans_mean_stat      |    15159      |   2204.08  |       0.224 | 38.26%  |
| gw2lob_trans_mean_stat     |    18253.2    |   1594.41  |       0.135 | 46.07%  |
| w2w_mean_stat              |    39618.8    |   4437.66  |       0.173 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |        mad |   robust cv |        MDE |   min-delta |
|:-------------------------|---------:|-----------:|------------:|-----------:|------------:|
| lob_intvl_apply_p99_stat |   2550.5 |     24.505 |       0.015 |     73.515 |       0.045 |
| w2w_p99_stat             | 277040   | 101115     |       0.562 | 303345     |       1.686 |
| throughput_stat          | 270496   |  13238.5   |       0.075 |  39715.5   |       0.225 |
### add
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2573 |    11   |       0.007 |    33   |       0.021 |
| w2w_p99_stat             |   247329 | 31813.5 |       0.198 | 95440.6 |       0.594 |
| throughput_stat          |   273362 | 14446   |       0.081 | 43338   |       0.243 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2506.5 |    31   |       0.019 |     93   |       0.057 |
| w2w_p99_stat             | 202044   | 64908.2 |       0.495 | 194725   |       1.485 |
| throughput_stat          | 271992   | 17464.5 |       0.099 |  52393.5 |       0.297 |
