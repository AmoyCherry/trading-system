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
| cross      | 0.29 ± 0.00       | 1.38 ± 0.59   | 2565.50 ± 35.00     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 0.98 ± 0.24   | 2594.00 ± 24.50     | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.82 ± 0.21   | 2523.00 ± 17.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2884.4    |     90.308 |       0.048 | 4.60%   |
| gw_intvl_decode_mean_stat  |       78.9694 |      3.388 |       0.066 | 0.13%   |
| gw_intvl_send_mean_stat    |     2829.24   |    138.493 |       0.075 | 4.51%   |
| lob_intvl_decode_mean_stat |       75.9525 |      3.787 |       0.077 | 0.12%   |
| lob_intvl_apply_mean_stat  |      300.199  |     15.214 |       0.078 | 0.48%   |
| ex2gw_trans_mean_stat      |    28964.6    |   9647.72  |       0.513 | 46.15%  |
| gw2lob_trans_mean_stat     |    29135.5    |   7875.14  |       0.416 | 46.42%  |
| w2w_mean_stat              |    62762.4    |  18407.4   |       0.452 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2764.03   |     25.871 |       0.014 | 5.11%   |
| gw_intvl_decode_mean_stat  |       74.1837 |      0.701 |       0.015 | 0.14%   |
| gw_intvl_send_mean_stat    |     2625.32   |     21.723 |       0.013 | 4.85%   |
| lob_intvl_decode_mean_stat |       72.2004 |      0.673 |       0.014 | 0.13%   |
| lob_intvl_apply_mean_stat  |      319.892  |      4.404 |       0.021 | 0.59%   |
| ex2gw_trans_mean_stat      |    22863.8    |   3583.95  |       0.241 | 42.27%  |
| gw2lob_trans_mean_stat     |    25434.2    |   4763.96  |       0.289 | 47.03%  |
| w2w_mean_stat              |    54085.4    |   8321.22  |       0.237 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2755      |     54.604 |       0.031 | 5.56%   |
| gw_intvl_decode_mean_stat  |       75.9089 |      1.356 |       0.028 | 0.15%   |
| gw_intvl_send_mean_stat    |     2610.44   |     35.097 |       0.021 | 5.27%   |
| lob_intvl_decode_mean_stat |       73.7733 |      0.924 |       0.019 | 0.15%   |
| lob_intvl_apply_mean_stat  |      291.829  |      4.645 |       0.025 | 0.59%   |
| ex2gw_trans_mean_stat      |    18921.8    |   3362.29  |       0.274 | 38.17%  |
| gw2lob_trans_mean_stat     |    24806.8    |   2527.77  |       0.157 | 50.05%  |
| w2w_mean_stat              |    49568.9    |   6713.02  |       0.209 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |      mad |   robust cv |             MDE |   min-delta |
|:-------------------------|-----------------:|---------:|------------:|----------------:|------------:|
| lob_intvl_apply_p99_stat |   2565.5         |     35   |       0.021 |   105           |       0.063 |
| w2w_p99_stat             |      1.37527e+06 | 593898   |       0.665 |     1.78169e+06 |       1.995 |
| throughput_stat          | 290018           |   4936.5 |       0.026 | 14809.5         |       0.078 |
### add
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2594 |     24.5 |       0.015 |     73.5 |       0.045 |
| w2w_p99_stat             |   975159 | 238112   |       0.376 | 714335   |       1.128 |
| throughput_stat          |   289630 |   4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |   median |      mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |     2523 |     17   |       0.01  |     51   |       0.03  |
| w2w_p99_stat             |   815168 | 208514   |       0.394 | 625541   |       1.182 |
| throughput_stat          |   290176 |   4163.5 |       0.022 |  12490.5 |       0.066 |
