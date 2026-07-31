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
| cross      | 0.29 ± 0.00       | 0.99 ± 0.07   | 2474.00 ± 37.50     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.00 ± 0.07   | 2510.00 ± 30.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 1.00 ± 0.06   | 2390.00 ± 23.50     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2376.01   |    100.657 |       0.065 | 0.42%   |
| gw_intvl_decode_mean_stat  |       71.2438 |      0.616 |       0.013 | 0.01%   |
| gw_intvl_send_mean_stat    |     1953.89   |     17.677 |       0.014 | 0.35%   |
| lob_intvl_decode_mean_stat |       69.3708 |      0.937 |       0.021 | 0.01%   |
| lob_intvl_apply_mean_stat  |      217.312  |      2.226 |       0.016 | 0.04%   |
| ex2gw_trans_mean_stat      |   549850      |  14125.2   |       0.04  | 97.47%  |
| gw2lob_trans_mean_stat     |     9694.59   |    129.3   |       0.021 | 1.72%   |
| w2w_mean_stat              |   564105      |  13625.2   |       0.037 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2351.68   |     69.349 |       0.045 | 0.42%   |
| gw_intvl_decode_mean_stat  |       72.0211 |      0.482 |       0.01  | 0.01%   |
| gw_intvl_send_mean_stat    |     1967.57   |     19.435 |       0.015 | 0.35%   |
| lob_intvl_decode_mean_stat |       68.8108 |      0.376 |       0.008 | 0.01%   |
| lob_intvl_apply_mean_stat  |      242.569  |      1.685 |       0.011 | 0.04%   |
| ex2gw_trans_mean_stat      |   546417      |   9728.49  |       0.027 | 97.45%  |
| gw2lob_trans_mean_stat     |     9900.57   |    427.66  |       0.067 | 1.77%   |
| w2w_mean_stat              |   560704      |   8660.27  |       0.024 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2367.49   |     85.715 |       0.056 | 0.42%   |
| gw_intvl_decode_mean_stat  |       73.3792 |      0.907 |       0.019 | 0.01%   |
| gw_intvl_send_mean_stat    |     1962.48   |     18.164 |       0.014 | 0.35%   |
| lob_intvl_decode_mean_stat |       70.3612 |      1.374 |       0.03  | 0.01%   |
| lob_intvl_apply_mean_stat  |      219.709  |      2.439 |       0.017 | 0.04%   |
| ex2gw_trans_mean_stat      |   545323      |   8663.35  |       0.024 | 97.29%  |
| gw2lob_trans_mean_stat     |    10459.4    |    946.547 |       0.139 | 1.87%   |
| w2w_mean_stat              |   560496      |   7986.7   |       0.022 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2474 |    37.5 |       0.023 |    112.5 |       0.069 |
| w2w_p99_stat             |   994386 | 74421.1 |       0.115 | 223263   |       0.345 |
| throughput_stat          |   290018 |  4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2510           |    30.5 |       0.019 |     91.5 |       0.057 |
| w2w_p99_stat             |      1.00415e+06 | 71109.6 |       0.109 | 213329   |       0.327 |
| throughput_stat          | 289630           |  4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2390           |    23.5 |       0.015 |     70.5 |       0.045 |
| w2w_p99_stat             |      1.00488e+06 | 55698   |       0.085 | 167094   |       0.255 |
| throughput_stat          | 290176           |  4163.5 |       0.022 |  12490.5 |       0.066 |
