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
| cross      | 0.29 ± 0.00       | 0.94 ± 0.03   | 2379.00 ± 33.50     | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.99 ± 0.07   | 2419.50 ± 14.50     | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 1.00 ± 0.08   | 2325.50 ± 24.00     | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3315.25   |     28.042 |       0.013 | 0.61%   |
| gw_intvl_decode_mean_stat  |       71.0407 |      0.942 |       0.02  | 0.01%   |
| gw_intvl_send_mean_stat    |     1919.35   |      9.067 |       0.007 | 0.35%   |
| lob_intvl_decode_mean_stat |       69.6689 |      0.534 |       0.012 | 0.01%   |
| lob_intvl_apply_mean_stat  |      206.954  |      1.72  |       0.013 | 0.04%   |
| ex2gw_trans_mean_stat      |   526635      |   6708.49  |       0.02  | 97.21%  |
| gw2lob_trans_mean_stat     |     9542.12   |    230.047 |       0.037 | 1.76%   |
| w2w_mean_stat              |   541753      |   6824.02  |       0.019 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3328.48   |     73.739 |       0.034 | 0.61%   |
| gw_intvl_decode_mean_stat  |       71.2438 |      0.657 |       0.014 | 0.01%   |
| gw_intvl_send_mean_stat    |     1932.98   |     34.993 |       0.028 | 0.35%   |
| lob_intvl_decode_mean_stat |       69.9275 |      0.53  |       0.012 | 0.01%   |
| lob_intvl_apply_mean_stat  |      239.014  |      1.987 |       0.013 | 0.04%   |
| ex2gw_trans_mean_stat      |   529780      |  12640.5   |       0.037 | 97.14%  |
| gw2lob_trans_mean_stat     |     9581.41   |    187.933 |       0.03  | 1.76%   |
| w2w_mean_stat              |   545370      |  12935.2   |       0.037 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     3333.33   |     67.75  |       0.031 | 0.60%   |
| gw_intvl_decode_mean_stat  |       72.0582 |      0.613 |       0.013 | 0.01%   |
| gw_intvl_send_mean_stat    |     1945.59   |     31.071 |       0.025 | 0.35%   |
| lob_intvl_decode_mean_stat |       71.3848 |      0.599 |       0.013 | 0.01%   |
| lob_intvl_apply_mean_stat  |      213.597  |      3.173 |       0.023 | 0.04%   |
| ex2gw_trans_mean_stat      |   537592      |  13972     |       0.04  | 97.28%  |
| gw2lob_trans_mean_stat     |     9694.54   |    392.952 |       0.062 | 1.75%   |
| w2w_mean_stat              |   552615      |  12835.4   |       0.036 | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |     mad |   robust cv |     MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|--------:|------------:|
| lob_intvl_apply_p99_stat |     2379 |    33.5 |       0.022 |   100.5 |       0.066 |
| w2w_p99_stat             |   937036 | 31225.1 |       0.051 | 93675.4 |       0.153 |
| throughput_stat          |   289306 |  3031.5 |       0.016 |  9094.5 |       0.048 |
### add
| metric                   |   median |     mad |   robust cv |      MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|---------:|------------:|
| lob_intvl_apply_p99_stat |   2419.5 |    14.5 |       0.009 |     43.5 |       0.027 |
| w2w_p99_stat             | 986248   | 72865.5 |       0.114 | 218596   |       0.342 |
| throughput_stat          | 280929   |  8467   |       0.046 |  25401   |       0.138 |
### cancel
| metric                   |   median |     mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|--------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |   2325.5 |    24   |       0.016 |     72 |       0.048 |
| w2w_p99_stat             | 999422   | 78486.5 |       0.121 | 235460 |       0.363 |
| throughput_stat          | 290380   |  3046   |       0.016 |   9138 |       0.048 |
