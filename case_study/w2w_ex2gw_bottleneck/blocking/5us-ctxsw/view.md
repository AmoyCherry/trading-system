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
| cross      | 0.29 ± 0.00       | 1.38 ± 0.17   | 2412.00 ± 28.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.43 ± 0.07   | 2470.51 ± 17.01     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 1.46 ± 0.10   | 2381.00 ± 28.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2433.04   |     44.285 |       0.028 | 2.99%   |
| gw_intvl_decode_mean_stat  |       73.0914 |      1.811 |       0.038 | 0.09%   |
| gw_intvl_send_mean_stat    |     2568.77   |     77.289 |       0.046 | 3.15%   |
| lob_intvl_decode_mean_stat |       70.5193 |      0.55  |       0.012 | 0.09%   |
| lob_intvl_apply_mean_stat  |      239.235  |      3.289 |       0.021 | 0.29%   |
| ex2gw_trans_mean_stat      |    55241      |  36280.2   |       1.012 | 67.78%  |
| gw2lob_trans_mean_stat     |    20013.9    |   2208.45  |       0.17  | 24.56%  |
| w2w_mean_stat              |    81504.9    |  42304.2   |       0.8   | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2433.86   |     35.593 |       0.023 | 3.30%   |
| gw_intvl_decode_mean_stat  |       73.4908 |      1.383 |       0.029 | 0.10%   |
| gw_intvl_send_mean_stat    |     2550.51   |     25.578 |       0.015 | 3.46%   |
| lob_intvl_decode_mean_stat |       71.501  |      0.646 |       0.014 | 0.10%   |
| lob_intvl_apply_mean_stat  |      278.803  |      2.884 |       0.016 | 0.38%   |
| ex2gw_trans_mean_stat      |    47404.2    |   9834.09  |       0.32  | 64.36%  |
| gw2lob_trans_mean_stat     |    20823.2    |   1224.7   |       0.091 | 28.27%  |
| w2w_mean_stat              |    73652.8    |  12412     |       0.26  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2415.2    |     22.447 |       0.014 | 2.38%   |
| gw_intvl_decode_mean_stat  |       76.6883 |      2.035 |       0.041 | 0.08%   |
| gw_intvl_send_mean_stat    |     2569.49   |     53.928 |       0.032 | 2.54%   |
| lob_intvl_decode_mean_stat |       72.5602 |      0.763 |       0.016 | 0.07%   |
| lob_intvl_apply_mean_stat  |      248.776  |      2.616 |       0.016 | 0.25%   |
| ex2gw_trans_mean_stat      |    75422.2    |  44371.1   |       0.906 | 74.42%  |
| gw2lob_trans_mean_stat     |    18576      |   1796.17  |       0.149 | 18.33%  |
| w2w_mean_stat              |   101344      |  45377     |       0.69  | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2412           |     28   |       0.018 |     84   |       0.054 |
| w2w_p99_stat             |      1.37509e+06 | 174744   |       0.196 | 524233   |       0.588 |
| throughput_stat          | 290018           |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |           median |       mad |   robust cv |        MDE |   min-delta |
|:-------------------------|-----------------:|----------:|------------:|-----------:|------------:|
| lob_intvl_apply_p99_stat |   2470.51        |    17.005 |       0.011 |     51.015 |       0.033 |
| w2w_p99_stat             |      1.43378e+06 | 72828     |       0.078 | 218484     |       0.234 |
| throughput_stat          | 289630           |  4221.5   |       0.022 |  12664.5   |       0.066 |
### cancel
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2381           |    28   |       0.018 |     84   |       0.054 |
| w2w_p99_stat             |      1.46473e+06 | 99004.4 |       0.104 | 297013   |       0.312 |
| throughput_stat          | 290176           |  4163.5 |       0.022 |  12490.5 |       0.066 |
