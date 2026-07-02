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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2367.00 ± 6.50      | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.01 ± 0.00   | 2427.00 ± 8.00      | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2342.50 ± 11.50     | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2125.48   |     12.838 |       0.009 | 10.47%  |
| gw_intvl_decode_mean_stat  |       71.4456 |      0.277 |       0.006 | 0.35%   |
| gw_intvl_send_mean_stat    |     2006.1    |     16.933 |       0.013 | 9.88%   |
| lob_intvl_decode_mean_stat |       69.4759 |      0.908 |       0.02  | 0.34%   |
| lob_intvl_apply_mean_stat  |      208.133  |      1.865 |       0.014 | 1.03%   |
| ex2gw_trans_mean_stat      |     7170.48   |    163.828 |       0.035 | 35.32%  |
| gw2lob_trans_mean_stat     |     8650.88   |    216.323 |       0.039 | 42.61%  |
| w2w_mean_stat              |    20303.9    |    347.18  |       0.026 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2164.38   |     31.3   |       0.022 | 9.74%   |
| gw_intvl_decode_mean_stat  |       72.5246 |      0.889 |       0.019 | 0.33%   |
| gw_intvl_send_mean_stat    |     2046.25   |     33.848 |       0.025 | 9.21%   |
| lob_intvl_decode_mean_stat |       69.945  |      0.918 |       0.02  | 0.31%   |
| lob_intvl_apply_mean_stat  |      248.382  |      4.587 |       0.028 | 1.12%   |
| ex2gw_trans_mean_stat      |     7900.06   |    513.14  |       0.1   | 35.56%  |
| gw2lob_trans_mean_stat     |     9169.48   |    692.16  |       0.116 | 41.28%  |
| w2w_mean_stat              |    22214.2    |    954.524 |       0.066 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2122.23   |     22.462 |       0.016 | 10.19%  |
| gw_intvl_decode_mean_stat  |       73.0753 |      0.464 |       0.01  | 0.35%   |
| gw_intvl_send_mean_stat    |     2001.38   |     11.458 |       0.009 | 9.61%   |
| lob_intvl_decode_mean_stat |       71.1792 |      0.478 |       0.01  | 0.34%   |
| lob_intvl_apply_mean_stat  |      219.154  |      1.108 |       0.008 | 1.05%   |
| ex2gw_trans_mean_stat      |     7192.3    |    176.239 |       0.038 | 34.52%  |
| gw2lob_trans_mean_stat     |     9162.03   |    690.481 |       0.116 | 43.98%  |
| w2w_mean_stat              |    20834.4    |    867.696 |       0.064 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2367   |    6.5 |       0.004 |   19.5 |       0.012 |
| w2w_p99_stat             |  10649.5 |  182.5 |       0.026 |  547.5 |       0.078 |
| throughput_stat          | 289306   | 3031.5 |       0.016 | 9094.5 |       0.048 |
### add
| metric                   |   median |    mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2427 |    8   |       0.005 |    24   |       0.015 |
| w2w_p99_stat             |    11054 |  285.5 |       0.04  |   856.5 |       0.12  |
| throughput_stat          |   280929 | 8467   |       0.046 | 25401   |       0.138 |
### cancel
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2342.5 |   11.5 |       0.008 |   34.5 |       0.024 |
| w2w_p99_stat             |  10804.5 |  193.5 |       0.028 |  580.5 |       0.084 |
| throughput_stat          | 290380   | 3046   |       0.016 | 9138   |       0.048 |
