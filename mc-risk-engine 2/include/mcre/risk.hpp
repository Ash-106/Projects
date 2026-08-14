#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace mcre {

struct RiskResult {
    double var        = 0.0;   // positive number = loss
    double es         = 0.0;
    double var_se     = 0.0;   // Monte Carlo standard error of the VaR estimate
    double es_se      = 0.0;
    double mean_pnl   = 0.0;
    std::size_t tail_n = 0;
};

// alpha = confidence level, e.g. 0.99 for 99% VaR.
// Uses nth_element (expected O(n)) rather than a full sort (O(n log n)): at 10^7 paths
// the quantile step is otherwise a visible fraction of total runtime.
inline RiskResult var_es(std::vector<double> pnl, double alpha) {
    RiskResult r;
    const std::size_t n = pnl.size();
    if (n == 0) return r;

    double sum = 0.0;
    for (double v : pnl) sum += v;
    r.mean_pnl = sum / static_cast<double>(n);

    const double p = 1.0 - alpha;
    std::size_t k = static_cast<std::size_t>(p * static_cast<double>(n));
    if (k == 0) k = 1;
    if (k >= n) k = n - 1;

    std::nth_element(pnl.begin(), pnl.begin() + k, pnl.end());
    r.var = -pnl[k];

    double tail = 0.0;
    for (std::size_t i = 0; i < k; ++i) tail += pnl[i];
    r.es = -tail / static_cast<double>(k);
    r.tail_n = k;

    double tv = 0.0;
    for (std::size_t i = 0; i < k; ++i) {
        const double d = -pnl[i] - r.es;
        tv += d * d;
    }
    r.es_se = std::sqrt(tv / static_cast<double>(k)) / std::sqrt(static_cast<double>(k));

    // Asymptotic quantile SE: sqrt(p(1-p)/n) / f(q), with the density estimated from the
    // spacing of order statistics around the quantile.
    const std::size_t m = std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt((double)n)));
    if (k > m && k + m < n) {
        std::nth_element(pnl.begin(), pnl.begin() + (k - m), pnl.begin() + k);
        const double lo = pnl[k - m];
        std::nth_element(pnl.begin() + k + 1, pnl.begin() + (k + m), pnl.end());
        const double hi = pnl[k + m];
        const double width = hi - lo;
        if (width > 0.0) {
            const double f = (2.0 * m / static_cast<double>(n)) / width;
            r.var_se = std::sqrt(p * (1.0 - p) / static_cast<double>(n)) / f;
        }
    }
    return r;
}

struct BacktestResult {
    std::size_t n = 0, exceptions = 0;
    double expected_rate = 0.0, observed_rate = 0.0;
    double lr_pof = 0.0, lr_ind = 0.0, lr_cc = 0.0;
    double crit_1df = 3.841, crit_2df = 5.991;  // chi2 at 95%
};

// hits[t] = 1 if realised loss exceeded that day's VaR forecast.
// Kupiec POF tests whether the exception *rate* matches 1-alpha; Christoffersen tests
// whether exceptions are independent over time. A model can pass the first and fail the
// second by clustering all its breaches in one stressed week, which is the failure mode
// that actually matters for a risk system.
inline BacktestResult backtest(const std::vector<int>& hits, double alpha) {
    BacktestResult b;
    b.n = hits.size();
    if (b.n == 0) return b;

    const double p = 1.0 - alpha;
    b.expected_rate = p;
    for (int h : hits) b.exceptions += static_cast<std::size_t>(h);
    const double x  = static_cast<double>(b.exceptions);
    const double T  = static_cast<double>(b.n);
    b.observed_rate = x / T;

    const double pi = x / T;
    if (x > 0 && x < T) {
        const double ll0 = (T - x) * std::log(1.0 - p)  + x * std::log(p);
        const double ll1 = (T - x) * std::log(1.0 - pi) + x * std::log(pi);
        b.lr_pof = -2.0 * (ll0 - ll1);
    }

    std::size_t n00 = 0, n01 = 0, n10 = 0, n11 = 0;
    for (std::size_t t = 1; t < b.n; ++t) {
        if (hits[t - 1] == 0 && hits[t] == 0) ++n00;
        else if (hits[t - 1] == 0 && hits[t] == 1) ++n01;
        else if (hits[t - 1] == 1 && hits[t] == 0) ++n10;
        else ++n11;
    }
    const double d0 = static_cast<double>(n00 + n01), d1 = static_cast<double>(n10 + n11);
    if (d0 > 0 && d1 > 0) {
        const double pi0 = n01 / d0, pi1 = n11 / d1;
        const double piu = static_cast<double>(n01 + n11) / (d0 + d1);
        auto lg = [](double v) { return v > 0.0 ? std::log(v) : 0.0; };
        const double llu = (d0 + d1 - (n01 + n11)) * lg(1.0 - piu) + (n01 + n11) * lg(piu);
        const double lla = n00 * lg(1.0 - pi0) + n01 * lg(pi0) + n10 * lg(1.0 - pi1) + n11 * lg(pi1);
        b.lr_ind = -2.0 * (llu - lla);
    }
    b.lr_cc = b.lr_pof + b.lr_ind;
    return b;
}

} // namespace mcre
