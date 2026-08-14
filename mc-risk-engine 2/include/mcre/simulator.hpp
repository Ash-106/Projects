#pragma once
#include "matrix.hpp"
#include "rng.hpp"
#include <vector>
#include <thread>
#include <algorithm>
#include <cmath>

namespace mcre {

enum class ShockModel { Gaussian, StudentT };

struct SimConfig {
    std::size_t   n_draws      = 1'000'000;   // independent shock draws
    double        horizon_days = 1.0;
    bool          antithetic   = false;       // each draw yields the pair (+z, -z)
    ShockModel    model        = ShockModel::Gaussian;
    double        nu           = 5.0;         // Student-t dof (must be > 2)
    std::uint64_t seed         = 0x5EEDULL;
    std::size_t   block        = 4096;
    unsigned      threads      = 1;
};

// Scenario count actually produced.
inline std::size_t scenario_count(const SimConfig& c) {
    return c.n_draws * (c.antithetic ? 2u : 1u);
}

// One-step GBM on log returns:
//   r_i = (mu_i - 0.5 * var_i) * h + sqrt(h) * x_i,   x = B z,  Cov(x) = Sigma
//   pnl = sum_i notional_i * (exp(r_i) - 1)
//
// Determinism: block b is seeded from f(seed, b) and writes to a fixed output slice, so
// results are bit-identical regardless of thread count. This is the property that makes
// a parallel MC engine debuggable and back-testable; per-thread seeding does not have it.
inline std::vector<double> simulate_pnl(const std::vector<double>& notional,
                                        const std::vector<double>& mu,
                                        const std::vector<double>& var,
                                        const Matrix& B,
                                        const SimConfig& cfg) {
    const std::size_t N    = notional.size();
    const std::size_t mult = cfg.antithetic ? 2 : 1;
    const double      h    = cfg.horizon_days;
    const double      sqh  = std::sqrt(h);
    // Cholesky gives a lower-triangular B, which halves the matvec flops in the hot loop.
    // The eigen-repair fallback gives a dense root, so detect rather than assume.
    bool lower_tri = true;
    for (std::size_t i = 0; i < N && lower_tri; ++i)
        for (std::size_t j = i + 1; j < N; ++j)
            if (B(i, j) != 0.0) { lower_tri = false; break; }

    std::vector<double> drift(N);
    for (std::size_t i = 0; i < N; ++i) drift[i] = (mu[i] - 0.5 * var[i]) * h;

    std::vector<double> out(cfg.n_draws * mult);
    const std::size_t n_blocks = (cfg.n_draws + cfg.block - 1) / cfg.block;
    const unsigned    nthreads = std::max(1u, cfg.threads);

    auto worker = [&](unsigned tid) {
        std::vector<double> z(N), x(N);
        for (std::size_t b = tid; b < n_blocks; b += nthreads) {
            std::uint64_t s = cfg.seed ^ (b * 0x9E3779B97F4A7C15ULL);
            Rng rng(splitmix64(s));

            const std::size_t begin = b * cfg.block;
            const std::size_t end   = std::min(begin + cfg.block, cfg.n_draws);

            for (std::size_t p = begin; p < end; ++p) {
                rng.clear_cache();
                for (std::size_t i = 0; i < N; ++i) z[i] = rng.normal();

                double tscale = 1.0;
                if (cfg.model == ShockModel::StudentT) {
                    // Variance-matched multivariate t: X = sqrt((nu-2)/W) * L Z, W ~ chi2_nu,
                    // so Cov(X) = Sigma while the marginals keep t_nu tails.
                    const double w = rng.chi2(cfg.nu);
                    tscale = std::sqrt((cfg.nu - 2.0) / w);
                }

                for (int sign_idx = 0; sign_idx < static_cast<int>(mult); ++sign_idx) {
                    const double sgn = (sign_idx == 0) ? 1.0 : -1.0;
                    for (std::size_t i = 0; i < N; ++i) {
                        double acc = 0.0;
                        const std::size_t kmax = lower_tri ? i + 1 : N;
                        for (std::size_t k = 0; k < kmax; ++k) acc += B(i, k) * z[k];
                        x[i] = sgn * tscale * acc;
                    }
                    double pnl = 0.0;
                    for (std::size_t i = 0; i < N; ++i)
                        pnl += notional[i] * std::expm1(drift[i] + sqh * x[i]);
                    out[p * mult + sign_idx] = pnl;
                }
            }
        }
    };

    if (nthreads == 1) {
        worker(0);
    } else {
        std::vector<std::thread> pool;
        pool.reserve(nthreads);
        for (unsigned t = 0; t < nthreads; ++t) pool.emplace_back(worker, t);
        for (auto& t : pool) t.join();
    }
    return out;
}

} // namespace mcre
