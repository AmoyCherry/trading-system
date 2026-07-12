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
| cross      | 0.29 ± 0.00       | 0.98 ± 0.11   | 2403.50 ± 23.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.05 ± 0.14   | 2419.51 ± 35.99     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.83 ± 0.23   | 2315.00 ± 10.50     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2256.87   |     36.234 |       0.025 | 4.31%   |
| gw_intvl_decode_mean_stat  |       72.1945 |      0.616 |       0.013 | 0.14%   |
| gw_intvl_send_mean_stat    |     2090.1    |     35.62  |       0.026 | 4.00%   |
| lob_intvl_decode_mean_stat |       69.9488 |      1.273 |       0.028 | 0.13%   |
| lob_intvl_apply_mean_stat  |      217.64   |      4.942 |       0.035 | 0.42%   |
| ex2gw_trans_mean_stat      |    36751.8    |  14334.3   |       0.601 | 70.25%  |
| gw2lob_trans_mean_stat     |    10076.1    |    442.294 |       0.068 | 19.26%  |
| w2w_mean_stat              |    52312.7    |  14953.4   |       0.44  | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2231.45   |     33.478 |       0.023 | 3.75%   |
| gw_intvl_decode_mean_stat  |       73.1355 |      1.625 |       0.034 | 0.12%   |
| gw_intvl_send_mean_stat    |     2111.21   |     34.949 |       0.026 | 3.55%   |
| lob_intvl_decode_mean_stat |       70.1667 |      1.335 |       0.029 | 0.12%   |
| lob_intvl_apply_mean_stat  |      246.907  |      2.518 |       0.016 | 0.41%   |
| ex2gw_trans_mean_stat      |    45340.3    |  12432.9   |       0.422 | 76.15%  |
| gw2lob_trans_mean_stat     |     9294.22   |    366.305 |       0.061 | 15.61%  |
| w2w_mean_stat              |    59539      |  12959.3   |       0.335 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2179.35   |     44.877 |       0.032 | 5.51%   |
| gw_intvl_decode_mean_stat  |       72.9645 |      0.553 |       0.012 | 0.18%   |
| gw_intvl_send_mean_stat    |     2076.9    |     35.395 |       0.026 | 5.25%   |
| lob_intvl_decode_mean_stat |       70.1112 |      0.46  |       0.01  | 0.18%   |
| lob_intvl_apply_mean_stat  |      217.858  |      1.709 |       0.012 | 0.55%   |
| ex2gw_trans_mean_stat      |    25261.3    |   5560.76  |       0.339 | 63.91%  |
| gw2lob_trans_mean_stat     |     9327.13   |    154.478 |       0.026 | 23.60%  |
| w2w_mean_stat              |    39528.3    |   6656.84  |       0.259 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2403.5 |     23   |       0.015 |     69   |       0.045 |
| w2w_p99_stat             | 981934   | 108113   |       0.17  | 324339   |       0.51  |
| throughput_stat          | 290018   |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |           median |        mad |   robust cv |        MDE |   min-delta |
|:-------------------------|-----------------:|-----------:|------------:|-----------:|------------:|
| lob_intvl_apply_p99_stat |   2419.51        |     35.995 |       0.023 |    107.985 |       0.069 |
| w2w_p99_stat             |      1.04673e+06 | 144245     |       0.212 | 432734     |       0.636 |
| throughput_stat          | 289630           |   4221.5   |       0.022 |  12664.5   |       0.066 |
### cancel
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2315 |     10.5 |       0.007 |     31.5 |       0.021 |
| w2w_p99_stat             |   831164 | 230987   |       0.428 | 692961   |       1.284 |
| throughput_stat          |   290176 |   4163.5 |       0.022 |  12490.5 |       0.066 |
