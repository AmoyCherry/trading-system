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
| cross      | 0.29 ± 0.00       | 0.01 ± 0.00   | 2409.00 ± 5.50      | 335.02, [260.38, 407.58]     | 33.23, [-39.39, 110.97]     | 2972.93 ± 36.52 |
| add        | 0.28 ± 0.01       | 0.01 ± 0.00   | 2474.50 ± 8.50      | 488.41, [402.83, 633.37]     | 70.17, [-31.88, 160.43]     | 2949.61 ± 67.29 |
| cancel     | 0.29 ± 0.00       | 0.01 ± 0.00   | 2365.00 ± 14.50     | 401.58, [357.03, 441.67]     | 31.70, [-9.42, 79.59]       | 2915.47 ± 17.44 |
## W2W Latency Decomposition

w2w decomp to answer which stage dominates the w2w latency and should be optimized.

> Why use `mean` to decompose?
> - Mathematics correctness. Percentiles (p99) are not **additive**, but `mean` is. `sigma(stage_mean) == w2w_mean`. But `sigma(stage_p99) != w2w_p99`.
> - The problem scope. LLN and CLT tell that `mean` is a high-quality metric when the repeats and samples large enough. But that's about estimator quality (why we didn't choose `mean` in Headline), w2w decomp is used to telescope stage percentages.
> - That's doesn't mean to `mean` is perfect for this problem. It's influenced by bad tails compare with median in a not-that-large sample, but it's additive while median not. No Free Lunch, regarding engineering for every step we must determine what to sacrifice.
### cross
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2137.13   |     19.788 |       0.014 | 10.69%  |
| gw_intvl_decode_mean_stat  |       70.9207 |      0.457 |       0.01  | 0.35%   |
| gw_intvl_send_mean_stat    |     1992.32   |      9.269 |       0.007 | 9.96%   |
| lob_intvl_decode_mean_stat |       70.1381 |      0.463 |       0.01  | 0.35%   |
| lob_intvl_apply_mean_stat  |      210.989  |      1.423 |       0.01  | 1.05%   |
| ex2gw_trans_mean_stat      |     6752.49   |     41.23  |       0.009 | 33.76%  |
| gw2lob_trans_mean_stat     |     8525.96   |    232.314 |       0.042 | 42.63%  |
| w2w_mean_stat              |    20001      |    483.159 |       0.037 | 100.00% |
### add
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2115.22   |     14.016 |       0.01  | 10.73%  |
| gw_intvl_decode_mean_stat  |       71.1003 |      0.737 |       0.016 | 0.36%   |
| gw_intvl_send_mean_stat    |     2020.02   |     18.672 |       0.014 | 10.24%  |
| lob_intvl_decode_mean_stat |       69.8961 |      0.615 |       0.014 | 0.35%   |
| lob_intvl_apply_mean_stat  |      245.123  |      1.643 |       0.01  | 1.24%   |
| ex2gw_trans_mean_stat      |     6759.78   |     59.866 |       0.014 | 34.28%  |
| gw2lob_trans_mean_stat     |     8437.72   |    103.622 |       0.019 | 42.78%  |
| w2w_mean_stat              |    19721.8    |    204.174 |       0.016 | 100.00% |
### cancel
| metric                     |   median (ns) |   mad (ns) |   robust cv | %w2w    |
|:---------------------------|--------------:|-----------:|------------:|:--------|
| ex_intvl_send_mean_stat    |     2127.8    |     15.442 |       0.011 | 10.74%  |
| gw_intvl_decode_mean_stat  |       72.3273 |      0.759 |       0.016 | 0.37%   |
| gw_intvl_send_mean_stat    |     1999.58   |     12.65  |       0.01  | 10.10%  |
| lob_intvl_decode_mean_stat |       71.0487 |      0.335 |       0.007 | 0.36%   |
| lob_intvl_apply_mean_stat  |      219.271  |      1.739 |       0.012 | 1.11%   |
| ex2gw_trans_mean_stat      |     6752.99   |     72.567 |       0.017 | 34.09%  |
| gw2lob_trans_mean_stat     |     8635.44   |    255.817 |       0.046 | 43.60%  |
| w2w_mean_stat              |    19806.8    |    260.629 |       0.02  | 100.00% |
## Noise Floor

> **Unbiased Robust CV**. The truth variance is systematically underestimated when dealing with a limited sample instead of the population (it's a infinite set in this case). While Bessel Correction (DDOF = 1) is for the `mean` family, we can use **Finite-sample Bias-correction Factors** to slightly expand the `mad` and `robust cv`.
>
> For repeats N = 10, define `Unbiased Robust CV = 1.4826 * 1.039 * mad / med`. Where `1.4826` is the Fisher-consistency constant and `1.039` is finite-sample bias-correction factor b(n) when n == 10.
### cross
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |     2409 |    5.5 |       0.004 |   16.5 |       0.012 |
| w2w_p99_stat             |    10161 |  162.5 |       0.025 |  487.5 |       0.075 |
| throughput_stat          |   289306 | 3031.5 |       0.016 | 9094.5 |       0.048 |
### add
| metric                   |   median |      mad |   robust cv |       MDE |   min-delta |
|:-------------------------|---------:|---------:|------------:|----------:|------------:|
| lob_intvl_apply_p99_stat |   2474.5 |    8.5   |       0.005 |    25.5   |       0.015 |
| w2w_p99_stat             |  10152.5 |  163.005 |       0.025 |   489.015 |       0.075 |
| throughput_stat          | 280929   | 8467     |       0.046 | 25401     |       0.138 |
### cancel
| metric                   |   median |    mad |   robust cv |    MDE |   min-delta |
|:-------------------------|---------:|-------:|------------:|-------:|------------:|
| lob_intvl_apply_p99_stat |     2365 |   14.5 |       0.009 |   43.5 |       0.027 |
| w2w_p99_stat             |    10006 |  144   |       0.022 |  432   |       0.066 |
| throughput_stat          |   290380 | 3046   |       0.016 | 9138   |       0.048 |
