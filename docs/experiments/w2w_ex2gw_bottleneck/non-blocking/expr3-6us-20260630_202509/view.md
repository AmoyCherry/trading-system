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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2417.50 ± 10.50     | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.01 ± 0.00   | 2550.00 ± 14.50     | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2465.50 ± 9.50      | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2161.87   |     11.552 |       0.008 | 9.05%   |
| gw_intvl_decode_mean_stat  |       71.1402 |      0.964 |       0.021 | 0.30%   |
| gw_intvl_send_mean_stat    |     2017.21   |     17.882 |       0.014 | 8.44%   |
| lob_intvl_decode_mean_stat |       70.1476 |      0.77  |       0.017 | 0.29%   |
| lob_intvl_apply_mean_stat  |      211.688  |      3.091 |       0.022 | 0.89%   |
| ex2gw_trans_mean_stat      |     8834.91   |    745.605 |       0.13  | 36.97%  |
| gw2lob_trans_mean_stat     |    10214      |    561.409 |       0.085 | 42.74%  |
| w2w_mean_stat              |    23899.4    |   1308.68  |       0.084 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2177.66   |     20.316 |       0.014 | 9.75%   |
| gw_intvl_decode_mean_stat  |       71.423  |      0.584 |       0.013 | 0.32%   |
| gw_intvl_send_mean_stat    |     2049.22   |     23.736 |       0.018 | 9.18%   |
| lob_intvl_decode_mean_stat |       70.6848 |      0.449 |       0.01  | 0.32%   |
| lob_intvl_apply_mean_stat  |      244.524  |      0.796 |       0.005 | 1.09%   |
| ex2gw_trans_mean_stat      |     8258.11   |    109.526 |       0.02  | 36.98%  |
| gw2lob_trans_mean_stat     |     9495      |    291.265 |       0.047 | 42.52%  |
| w2w_mean_stat              |    22332.7    |    439.658 |       0.03  | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2150.22   |      8.089 |       0.006 | 9.20%   |
| gw_intvl_decode_mean_stat  |       71.9942 |      0.453 |       0.01  | 0.31%   |
| gw_intvl_send_mean_stat    |     2062.12   |     38.552 |       0.029 | 8.83%   |
| lob_intvl_decode_mean_stat |       71.7706 |      0.473 |       0.01  | 0.31%   |
| lob_intvl_apply_mean_stat  |      221.136  |      0.763 |       0.005 | 0.95%   |
| ex2gw_trans_mean_stat      |     8930.38   |    336.27  |       0.058 | 38.22%  |
| gw2lob_trans_mean_stat     |     9601.41   |    373.624 |       0.06  | 41.10%  |
| w2w_mean_stat              |    23363.5    |    505.159 |       0.033 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2417.5 |   10.5 |       0.007 |   31.5 |       0.021 |
| w2w_p99_stat             |  11104.5 |  257   |       0.036 |  771   |       0.108 |
| throughput_stat          | 289306   | 3031.5 |       0.016 | 9094.5 |       0.048 |
### add
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2550   |   14.5 |       0.009 |    43.5 |       0.027 |
| w2w_p99_stat             |  11278.5 |   91.5 |       0.012 |   274.5 |       0.036 |
| throughput_stat          | 280929   | 8467   |       0.046 | 25401   |       0.138 |
### cancel
| metric                   |   median |      mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |   2465.5 |    9.5   |       0.006 |   28.5  |       0.018 |
| w2w_p99_stat             |  11557.5 |  456.505 |       0.061 | 1369.51 |       0.183 |
| throughput_stat          | 290380   | 3046     |       0.016 | 9138    |       0.048 |
