# Monte Carlo Risk Engine

A multi-threaded C++20 engine that estimates one-day portfolio VaR and Expected Shortfall
by simulation, and — more importantly — **checks whether those estimates are actually
calibrated** using Kupiec and Christoffersen backtests.

The interesting question in this repo is not "how do I run Monte Carlo". It is: *given
that every risk model is wrong, which estimator choices survive a statistical test on
out-of-sample data, and what do they cost in runtime?*

```
prices → log returns → covariance estimator → matrix factorization
                                                     ↓
                       parallel scenario generation (Gaussian / Student-t, antithetic)
                                                     ↓
                             P&L distribution → VaR, ES, standard errors
                                                     ↓
                                    rolling backtest → Kupiec / Christoffersen
```

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
./build/mcre vr   --prices data/prices.csv --draws 200000 --reps 24
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

Synthetic data with volatility clustering, 8 assets, ₹1 crore equal-weighted, 500-day
rolling window, 99% VaR, 1,999 out-of-sample days. Expected exceptions: 20.

| Covariance | Shocks | Exceptions | Kupiec LR | Christoffersen LR | Joint |
|---|---|---|---|---|---|
| sample | Gaussian | 28 | 2.88 pass | 3.49 pass | 6.38 **fail** |
| sample | Student-t (ν=5) | 13 | 2.82 pass | 0.17 pass | 2.99 pass |
| EWMA | Gaussian | 32 | 6.17 **fail** | 6.03 **fail** | 12.20 **fail** |
| EWMA | Student-t (ν=5) | **21** | 0.05 pass | 0.45 pass | 0.50 pass |

Two things worth noting. Gaussian shocks under-cover at the 99% level even when the
covariance is estimated well — the breaches are not just too frequent but *clustered*,
which is the failure mode that matters operationally. And EWMA makes the Gaussian model
strictly worse before it makes it better: reacting faster to volatility means the model
also shrinks its VaR faster after a quiet stretch, and without fat tails that is a
liability. Only the combination is calibrated.

Antithetic variates, equal scenario budget (200k), 24 independent replications:

| Estimator | sd(VaR) | sd(ES) | Variance ratio |
|---|---|---|---|
| Plain MC | 432.2 | 505.4 | — |
| Antithetic | 330.4 | 442.9 | 1.71× (VaR), 1.30× (ES) |

Less than the textbook speedup, and that is expected: antithetic sampling works by
cancelling the linear component of the estimator, and a 99% tail quantile is dominated by
its nonlinear part. It helps ES less than VaR for the same reason.

Throughput, single core (Intel, `-O3 -march=native`), 8 assets: **7.5M scenarios/s**
(≈60M asset returns/s), with the quantile and ES pass adding ~13% on top. Run
`mcre bench` for the scaling table on your own machine.

## Limitations

- One-step GBM. No path dependence, so this prices a one-day horizon honestly and a
  multi-day horizon only under a square-root-of-time assumption that is known to be wrong
  when volatility is persistent.
- Covariance is conditioned on a trailing window, not modelled. GARCH or a DCC model
  would be the next step, and would likely dominate EWMA in the backtest.
- Linear positions only. Options would need revaluation per scenario rather than a
  weighted sum of returns.
- The headline table is synthetic data. Numbers on real equities will differ; the
  backtest harness is the point, not the specific figures.

## Layout

```
include/mcre/matrix.hpp     Matrix, Cholesky, Jacobi eigendecomposition, PSD repair
include/mcre/stats.hpp      log returns, sample/EWMA/Ledoit-Wolf covariance
include/mcre/rng.hpp        xoshiro256++, normal (polar), gamma (Marsaglia-Tsang)
include/mcre/simulator.hpp  parallel scenario generation, block-deterministic seeding
include/mcre/risk.hpp       VaR/ES with standard errors, Kupiec, Christoffersen
src/main.cpp                CLI: gen | run | bench | vr | backtest
tests/test_core.cpp         factorization, MC vs closed form, t-moments, determinism
tools/fetch_prices.py       real price history → CSV
```
