## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
```

Header-only core (`include/mcre/`), no external dependencies.

## Quick start

```bash
./build/mcre gen --assets 8 --days 2500 --out data/prices.csv     # synthetic data
./build/mcre run --prices data/prices.csv --draws 4000000 \
      --cov ewma --shocks t --nu 5 --antithetic --alpha 0.99 --threads 8
./build/mcre backtest --prices data/prices.csv --cov ewma --shocks t --window 500
./build/mcre bench --prices data/prices.csv --draws 20000000 --max-threads 16
./build/mcre vr   --prices data/prices.csv --draws 200000 --reps 400
```

For real data: `python3 tools/fetch_prices.py --tickers ... --out data/real.csv`.

## What it does

**Covariance estimation** — three estimators, selectable at runtime:
`sample` (equal-weighted), `ewma` (RiskMetrics, λ = 0.94), and `lw`
(Ledoit–Wolf shrinkage toward a scaled identity, with the shrinkage intensity computed
from the data rather than hand-tuned). Sample covariance is badly conditioned when the
number of assets is comparable to the window length; shrinkage both fixes the
conditioning and reduces estimation error.

**Factorization** — Cholesky where possible (lower-triangular, so the shock matvec costs
half the flops of a dense root), with an eigenvalue-clipping fallback via cyclic Jacobi
when the estimate is singular or near-singular. The pivot test is *relative* to the
matrix scale: a naive `if (pivot <= 0)` check happily accepts a pivot of 1e-18 on a
matrix scaled at 1e-2 and returns a factor whose trailing columns are rounding noise.

**Scenario generation** — one-step GBM on log returns,
`r = (μ − σ²/2)h + √h · (Bz)`, with Gaussian or variance-matched multivariate-t shocks
(`X = √((ν−2)/W) · Bz`, `W ~ χ²_ν`, so the covariance target is preserved while the
marginals keep t tails). Optional antithetic variates.

**Parallelism with reproducibility** — work is split into fixed-size blocks; block *b* is
seeded from `f(seed, b)` and writes to a fixed output slice. Results are **bit-identical
regardless of thread count**, which is what makes a stochastic engine debuggable and its
backtests reproducible. Per-thread seeding, the obvious approach, does not have this
property.

**Risk metrics** — VaR and ES via `nth_element` (expected O(n)) rather than a full sort;
at 10⁷ paths the quantile step is otherwise a visible fraction of total runtime. Both
metrics are reported with Monte Carlo standard errors — the VaR error uses the asymptotic
quantile formula with the density estimated from order-statistic spacings — so it is
clear when a difference between two models is real and when it is simulation noise.

**Backtesting** — rolling-window, out-of-sample: refit the covariance on the trailing
window, simulate, compare the forecast VaR against the realised P&L, and test the
resulting exception sequence with Kupiec POF (is the *rate* right?) and Christoffersen
independence (are exceptions *clustered*?).

## Results

**Data:** 8 NSE large caps (RELIANCE, HDFCBANK, INFY, TCS, ICICIBANK, ITC, LT, SBIN),
2015-2026, 2,872 daily observations. Rs 1 crore equal-weighted, 500-day rolling window,
99% one-day VaR, 2,371 out-of-sample days. Expected exceptions: 23.7.

| Covariance | Shocks | Exceptions | Kupiec (rate) | Christoffersen (clustering) | Joint |
|---|---|---|---|---|---|
| sample | Gaussian | 28 | 0.74 pass | **19.50 fail** | 20.24 fail |
| sample | Student-t (v=5) | 20 | 0.62 pass | 1.97 pass | 2.59 pass |
| EWMA | Gaussian | 43 | **12.78 fail** | 0.06 pass | 12.83 fail |
| EWMA | Student-t (v=5) | **24** | **0.004 pass** | 0.49 pass | 0.50 pass |

The two failure modes are orthogonal, which is the point of running both tests.

Equal-weighted covariance passes on breach *rate* and fails independence badly (LR = 19.5):
it reacts too slowly to volatility, so breaches arrive in clusters around stress episodes.
A Kupiec test alone would have passed this model.

EWMA fixes the clustering almost completely (19.50 -> 0.06) and simultaneously makes the
rate much worse (43 breaches, 1.81% against a 1% target). Reacting quickly to volatility
is not enough when the shock distribution has tails that are too thin.

Only the combination is calibrated on both axes: EWMA governs *when* breaches occur,
Student-t governs *how often*.

### Antithetic variates: a measured null result

Equal scenario budget (200k), 400 independent replications, same data:

| Estimator | sd(plain) | sd(antithetic) | Variance ratio | 95% CI | Verdict |
|---|---|---|---|---|---|
| mean P&L | 228.8 | 3.21 | 5077x | [4173, 6178] | reduces variance |
| VaR 99% | 848.8 | 880.5 | 0.93x | [0.76, 1.13] | no detectable effect |
| ES 99% | 1082 | 1083 | 1.00x | [0.82, 1.21] | no detectable effect |

Antithetic sampling cancels the odd component of an estimator. The sample mean is nearly
linear in the shocks, so it benefits enormously - the 5000x figure is the control that
confirms the implementation is correct. A far-tail quantile does not benefit: an
antithetic pair can essentially never place both members in the 1% tail, so there is no
cancellation where the estimator actually lives. Writing the indicator covariance out
gives Var proportional to p(1-2p)/n against p(1-p)/n, a predicted gain of 1.01x at
p = 0.01, consistent with what is measured.

This is why the tool reports confidence intervals. At 24 replications the interval on the
VaR ratio spans roughly [0.55, 2.80], wide enough to "show" a 1.7x improvement that does
not exist. The null result is the finding.

### Throughput

4-core ARM64 VM (Apple Silicon host), `-O3 -march=native`, 8 assets, 20M scenarios:

| Threads | Runtime (s) | M scenarios/s | Speedup | VaR checksum |
|---|---|---|---|---|
| 1 | 1.786 | 11.20 | 1.00x | 249843 |
| 2 | 0.916 | 21.85 | 1.95x | 249843 |
| 4 | 0.497 | 40.28 | 3.60x | 249843 |

90% parallel efficiency at 4 threads. The identical checksums are the block-deterministic
seeding working; the same figure appears on x86-64, so results are reproducible across
architectures as well as across thread counts.

## Limitations

- One-step GBM. No path dependence, so this prices a one-day horizon honestly and a
  multi-day horizon only under a square-root-of-time assumption that is known to be wrong
  when volatility is persistent.
- Covariance is conditioned on a trailing window, not modelled. GARCH or a DCC model
  would be the next step, and would likely dominate EWMA in the backtest.
- Linear positions only. Options would need revaluation per scenario rather than a
  weighted sum of returns.
- Results are for 8 Indian large caps over one particular decade. The conclusion that
  EWMA plus fat tails is the calibrated combination should not be assumed to transfer to
  other markets, asset classes, or periods without rerunning the backtest.
- `nu = 5` is fixed rather than fitted. A profiled or per-asset estimate would be more
  principled, and the model is somewhat sensitive to it.

## Layout
