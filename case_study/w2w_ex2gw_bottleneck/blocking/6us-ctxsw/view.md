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
| cross      | 0.29 ± 0.00       | 1.23 ± 0.27   | 2502.00 ± 29.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.81 ± 0.15   | 2595.50 ± 19.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.64 ± 0.07   | 2462.00 ± 11.50     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2551.12   |     48.155 |       0.029 | 4.05%   |
| gw_intvl_decode_mean_stat  |       75.4952 |      3.432 |       0.07  | 0.12%   |
| gw_intvl_send_mean_stat    |     2490.72   |     71.094 |       0.044 | 3.95%   |
| lob_intvl_decode_mean_stat |       72.5501 |      0.832 |       0.018 | 0.12%   |
| lob_intvl_apply_mean_stat  |      257.267  |      3.242 |       0.019 | 0.41%   |
| ex2gw_trans_mean_stat      |    31464.9    |   5782.71  |       0.283 | 49.94%  |
| gw2lob_trans_mean_stat     |    24316.2    |   3379.64  |       0.214 | 38.59%  |
| w2w_mean_stat              |    63009.7    |  12396     |       0.303 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2543.64   |     28.308 |       0.017 | 4.81%   |
| gw_intvl_decode_mean_stat  |       76.2952 |      2.131 |       0.043 | 0.14%   |
| gw_intvl_send_mean_stat    |     2540.48   |     92.68  |       0.056 | 4.81%   |
| lob_intvl_decode_mean_stat |       71.5878 |      0.456 |       0.01  | 0.14%   |
| lob_intvl_apply_mean_stat  |      290.949  |      1.408 |       0.007 | 0.55%   |
| ex2gw_trans_mean_stat      |    24279.4    |   1242.99  |       0.079 | 45.95%  |
| gw2lob_trans_mean_stat     |    22782.8    |    630.99  |       0.043 | 43.12%  |
| w2w_mean_stat              |    52839.5    |   1447.14  |       0.042 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |      2531.03  |     17.554 |       0.011 | 5.37%   |
| gw_intvl_decode_mean_stat  |        77.446 |      2.612 |       0.052 | 0.16%   |
| gw_intvl_send_mean_stat    |      2563.97  |     78.67  |       0.047 | 5.44%   |
| lob_intvl_decode_mean_stat |        72.919 |      0.764 |       0.016 | 0.15%   |
| lob_intvl_apply_mean_stat  |       259.862 |      1.89  |       0.011 | 0.55%   |
| ex2gw_trans_mean_stat      |     21347.8   |   1197.93  |       0.086 | 45.30%  |
| gw2lob_trans_mean_stat     |     19500.3   |   1149.01  |       0.091 | 41.38%  |
| w2w_mean_stat              |     47125.6   |   2224.32  |       0.073 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2502           |     29   |       0.018 |     87   |       0.054 |
| w2w_p99_stat             |      1.23402e+06 | 266680   |       0.333 | 800039   |       0.999 |
| throughput_stat          | 290018           |   4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2595.5 |     19.5 |       0.012 |     58.5 |       0.036 |
| w2w_p99_stat             | 806890   | 145699   |       0.278 | 437096   |       0.834 |
| throughput_stat          | 289630   |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2462 |    11.5 |       0.007 |     34.5 |       0.021 |
| w2w_p99_stat             |   635917 | 74152.6 |       0.18  | 222458   |       0.54  |
| throughput_stat          |   290176 |  4163.5 |       0.022 |  12490.5 |       0.066 |
