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
| cross      | 0.29 ± 0.00       | 1.06 ± 0.10   | 2371.50 ± 13.50     | 370.16, [301.93, 426.41]     | 36.98, [-24.48, 99.88]      | 2950.29 ± 54.55 |
| add        | 0.29 ± 0.00       | 1.00 ± 0.08   | 2410.50 ± 7.50      | 551.90, [502.25, 612.47]     | 33.10, [-13.17, 68.98]      | 2939.14 ± 22.14 |
| cancel     | 0.29 ± 0.00       | 0.96 ± 0.03   | 2338.50 ± 21.00     | 392.24, [346.83, 446.52]     | 56.34, [3.24, 105.38]       | 2930.76 ± 35.75 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3371.99   |     58.293 |       0.027 | 0.61%   |
| gw_intvl_decode_mean_stat  |       71.7912 |      0.828 |       0.018 | 0.01%   |
| gw_intvl_send_mean_stat    |     1979.51   |     21.475 |       0.017 | 0.36%   |
| lob_intvl_decode_mean_stat |       69.9212 |      1.118 |       0.025 | 0.01%   |
| lob_intvl_apply_mean_stat  |      216.909  |      6.596 |       0.047 | 0.04%   |
| ex2gw_trans_mean_stat      |   536993      |   9242.51  |       0.027 | 97.14%  |
| gw2lob_trans_mean_stat     |    10231.6    |    641.546 |       0.097 | 1.85%   |
| w2w_mean_stat              |   552797      |   9125.28  |       0.025 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3396.09   |     74.204 |       0.034 | 0.61%   |
| gw_intvl_decode_mean_stat  |       72.0353 |      0.85  |       0.018 | 0.01%   |
| gw_intvl_send_mean_stat    |     1973.41   |     38.723 |       0.03  | 0.36%   |
| lob_intvl_decode_mean_stat |       69.376  |      1.261 |       0.028 | 0.01%   |
| lob_intvl_apply_mean_stat  |      240.102  |      4.113 |       0.026 | 0.04%   |
| ex2gw_trans_mean_stat      |   538881      |  13190.7   |       0.038 | 97.23%  |
| gw2lob_trans_mean_stat     |     9238.06   |    147.196 |       0.025 | 1.67%   |
| w2w_mean_stat              |   554215      |  13604.6   |       0.038 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3356.32   |     33.76  |       0.015 | 0.61%   |
| gw_intvl_decode_mean_stat  |       72.5228 |      0.401 |       0.009 | 0.01%   |
| gw_intvl_send_mean_stat    |     1952.29   |     12.268 |       0.01  | 0.35%   |
| lob_intvl_decode_mean_stat |       70.5576 |      0.567 |       0.012 | 0.01%   |
| lob_intvl_apply_mean_stat  |      222.317  |      1.823 |       0.013 | 0.04%   |
| ex2gw_trans_mean_stat      |   536451      |   6400.43  |       0.018 | 97.17%  |
| gw2lob_trans_mean_stat     |     9964.25   |    373.06  |       0.058 | 1.80%   |
| w2w_mean_stat              |   552083      |   7066.61  |       0.02  | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |           median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|-----------------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2371.5         |    13.5 |       0.009 |     40.5 |       0.027 |
| w2w_p99_stat             |      1.05557e+06 | 97328   |       0.142 | 291984   |       0.426 |
| throughput_stat          | 290018           |  4936.5 |       0.026 |  14809.5 |       0.078 |
### add
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2410.5 |     7.5 |       0.005 |     22.5 |       0.015 |
| w2w_p99_stat             | 999502   | 81885.5 |       0.126 | 245656   |       0.378 |
| throughput_stat          | 289630   |  4221.5 |       0.022 |  12664.5 |       0.066 |
### cancel
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2338.5 |    21   |       0.014 |    63   |       0.042 |
| w2w_p99_stat             | 955455   | 27301.6 |       0.044 | 81904.7 |       0.132 |
| throughput_stat          | 290176   |  4163.5 |       0.022 | 12490.5 |       0.066 |
