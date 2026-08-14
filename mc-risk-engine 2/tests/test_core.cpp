#include "mcre/matrix.hpp"
#include "mcre/stats.hpp"
#include "mcre/simulator.hpp"
#include "mcre/risk.hpp"

#include <cstdio>
#include <cmath>
#include <vector>

using namespace mcre;

static int failures = 0;

static void check(bool ok, const char* name, const std::string& detail = "") {
    std::printf("[%s] %s%s%s\n", ok ? "PASS" : "FAIL", name,
                detail.empty() ? "" : "  ", detail.c_str());
    if (!ok) ++failures;
}

static bool close(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// B B^T should reproduce Sigma
static double recon_error(const Matrix& Sigma, const Matrix& B) {
    const std::size_t n = Sigma.rows();
    double worst = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            double acc = 0.0;
            for (std::size_t k = 0; k < n; ++k) acc += B(i, k) * B(j, k);
            worst = std::max(worst, std::fabs(acc - Sigma(i, j)));
        }
    return worst;
}

int main() {
    // 1. Cholesky on a well-conditioned PD matrix
    {
        Matrix S(3, 3);
        const double v[3] = {0.02, 0.015, 0.03};
        const double rho[3][3] = {{1.0, 0.8, 0.2}, {0.8, 1.0, 0.3}, {0.2, 0.3, 1.0}};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) S(i, j) = rho[i][j] * v[i] * v[j];
        auto f = factorize(S);
        check(f.used_cholesky && recon_error(S, f.B) < 1e-15, "cholesky reconstructs Sigma");
    }

    // 2. Singular PSD matrix must fall back and still reproduce Sigma
    {
        Matrix S(3, 3);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) S(i, j) = 0.04;   // rank 1
        auto f = factorize(S);
        check(!f.used_cholesky && recon_error(S, f.B) < 1e-12,
              "rank-deficient Sigma -> eigen repair", "clipped=" + std::to_string(f.clipped));
    }

    // 3. Simulated shocks reproduce the target covariance
    {
        const std::size_t N = 3;
        Matrix S(N, N);
        const double v[3] = {0.02, 0.015, 0.03};
        const double rho[3][3] = {{1.0, 0.8, 0.2}, {0.8, 1.0, 0.3}, {0.2, 0.3, 1.0}};
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j) S(i, j) = rho[i][j] * v[i] * v[j];
        auto f = factorize(S);

        // isolate asset i by holding a unit notional in it alone
        double worst = 0.0;
        for (std::size_t i = 0; i < N; ++i) {
            std::vector<double> notional(N, 0.0), mu(N, 0.0), var = diag(S);
            notional[i] = 1.0;
            SimConfig c; c.n_draws = 400000; c.seed = 7;
            auto pnl = simulate_pnl(notional, mu, var, f.B, c);
            double m = 0.0; for (double x : pnl) m += x; m /= pnl.size();
            double s = 0.0; for (double x : pnl) s += (x - m) * (x - m);
            s = std::sqrt(s / (pnl.size() - 1));
            worst = std::max(worst, std::fabs(s - v[i]) / v[i]);
        }
        check(worst < 0.01, "simulated marginal vols match Sigma",
              "max rel err " + std::to_string(worst));
    }

    // 4. Monte Carlo VaR agrees with the closed-form Gaussian answer
    {
        const double sigma = 0.02, notional_ = 1e7, z99 = 2.3263478740408408;
        Matrix S(1, 1); S(0, 0) = sigma * sigma;
        auto f = factorize(S);
        std::vector<double> nt{notional_}, mu{0.0}, var{sigma * sigma};
        SimConfig c; c.n_draws = 4000000; c.seed = 11; c.antithetic = true;
        auto r = var_es(simulate_pnl(nt, mu, var, f.B, c), 0.99);
        const double analytic = -notional_ * std::expm1(-0.5 * sigma * sigma - z99 * sigma);
        check(close(r.var, analytic, 4.0 * r.var_se + 1.0), "MC VaR matches closed form",
              "mc=" + std::to_string(r.var) + " analytic=" + std::to_string(analytic));
    }

    // 5. Variance-matched Student-t keeps the target variance but fattens the tail
    {
        const double sigma = 0.02;
        Matrix S(1, 1); S(0, 0) = sigma * sigma;
        auto f = factorize(S);
        std::vector<double> nt{1.0}, mu{0.0}, var{sigma * sigma};
        SimConfig g; g.n_draws = 2000000; g.seed = 3;
        SimConfig t = g; t.model = ShockModel::StudentT; t.nu = 4.0;

        auto sd = [](const std::vector<double>& v) {
            double m = 0.0; for (double x : v) m += x; m /= v.size();
            double s = 0.0; for (double x : v) s += (x - m) * (x - m);
            return std::sqrt(s / (v.size() - 1));
        };
        auto pg = simulate_pnl(nt, mu, var, f.B, g);
        auto pt = simulate_pnl(nt, mu, var, f.B, t);
        const double sg = sd(pg), st = sd(pt);
        const double vg = var_es(pg, 0.995).var, vt = var_es(pt, 0.995).var;
        check(std::fabs(st - sg) / sg < 0.02 && vt > vg * 1.15,
              "student-t: same variance, fatter tail",
              "sd ratio " + std::to_string(st / sg) + ", VaR995 ratio " + std::to_string(vt / vg));
    }

    // 6. var_es on a known distribution
    {
        std::vector<double> pnl(1000);
        for (int i = 0; i < 1000; ++i) pnl[i] = -static_cast<double>(1000 - i);  // -1000 .. -1
        auto r = var_es(pnl, 0.99);          // k = 10 -> 10th smallest is -991
        check(close(r.var, 990.0, 1e-9) && close(r.es, 995.5, 1e-9), "var_es on known sample",
              "var=" + std::to_string(r.var) + " es=" + std::to_string(r.es));
    }

    // 7. Thread count must not change the answer
    {
        Matrix S(2, 2);
        S(0, 0) = 4e-4; S(1, 1) = 9e-4; S(0, 1) = S(1, 0) = 0.5 * 0.02 * 0.03;
        auto f = factorize(S);
        std::vector<double> nt{1e6, 1e6}, mu{0.0, 0.0}, var = diag(S);
        SimConfig a; a.n_draws = 500000; a.seed = 99; a.threads = 1;
        SimConfig b = a; b.threads = 8;
        auto pa = simulate_pnl(nt, mu, var, f.B, a);
        auto pb = simulate_pnl(nt, mu, var, f.B, b);
        bool same = pa.size() == pb.size();
        for (std::size_t i = 0; same && i < pa.size(); ++i) same = (pa[i] == pb[i]);
        check(same, "results are bit-identical across thread counts");
    }

    // 8. Kupiec: a correctly calibrated hit sequence should not be rejected
    {
        std::vector<int> hits(1000, 0);
        for (int i = 0; i < 10; ++i) hits[i * 97] = 1;   // 1% rate, spread out
        auto b = backtest(hits, 0.99);
        check(b.lr_pof < 3.841 && b.lr_ind < 3.841, "backtest accepts a calibrated sequence",
              "pof=" + std::to_string(b.lr_pof) + " ind=" + std::to_string(b.lr_ind));
    }

    // 9. Kupiec: a badly calibrated sequence should be rejected
    {
        std::vector<int> hits(1000, 0);
        for (int i = 0; i < 60; ++i) hits[i * 16] = 1;   // 6% rate
        auto b = backtest(hits, 0.99);
        check(b.lr_pof > 3.841, "backtest rejects an over-optimistic model",
              "pof=" + std::to_string(b.lr_pof));
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL TESTS PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
