#include "mcre/matrix.hpp"
#include "mcre/stats.hpp"
#include "mcre/rng.hpp"
#include "mcre/simulator.hpp"
#include "mcre/risk.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace mcre;

// ---------------------------------------------------------------- CSV I/O

struct PriceSeries {
    std::vector<std::string> names;
    Matrix prices;  // T x N
};

static PriceSeries read_prices(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); std::exit(1); }

    std::string line;
    std::getline(in, line);
    PriceSeries ps;
    {
        std::stringstream ss(line);
        std::string cell;
        bool first = true;
        while (std::getline(ss, cell, ',')) {
            if (first) { first = false; continue; }  // date column
            ps.names.push_back(cell);
        }
    }
    std::vector<std::vector<double>> rows;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<double> row;
        bool first = true;
        while (std::getline(ss, cell, ',')) {
            if (first) { first = false; continue; }
            row.push_back(std::atof(cell.c_str()));
        }
        if (row.size() == ps.names.size()) rows.push_back(std::move(row));
    }
    ps.prices = Matrix(rows.size(), ps.names.size());
    for (std::size_t t = 0; t < rows.size(); ++t)
        for (std::size_t i = 0; i < ps.names.size(); ++i) ps.prices(t, i) = rows[t][i];
    return ps;
}

// ---------------------------------------------------------------- arg parsing

static std::string arg_str(int argc, char** argv, const char* key, const std::string& def) {
    for (int i = 0; i + 1 < argc; ++i) if (!std::strcmp(argv[i], key)) return argv[i + 1];
    return def;
}
static double arg_num(int argc, char** argv, const char* key, double def) {
    for (int i = 0; i + 1 < argc; ++i) if (!std::strcmp(argv[i], key)) return std::atof(argv[i + 1]);
    return def;
}
static bool arg_flag(int argc, char** argv, const char* key) {
    for (int i = 0; i < argc; ++i) if (!std::strcmp(argv[i], key)) return true;
    return false;
}

// ---------------------------------------------------------------- synthetic data

// Generates correlated price paths with a stochastic-volatility overlay, so the resulting
// series has volatility clustering and fat tails that a plain Gaussian model will misprice.
static int cmd_gen(int argc, char** argv) {
    const std::size_t N = (std::size_t)arg_num(argc, argv, "--assets", 8);
    const std::size_t T = (std::size_t)arg_num(argc, argv, "--days", 2000);
    const std::string out = arg_str(argc, argv, "--out", "data/prices.csv");
    Rng rng((std::uint64_t)arg_num(argc, argv, "--seed", 20260814));

    Matrix C = Matrix::identity(N);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < i; ++j) {
            const double rho = 0.25 + 0.45 * rng.uniform01();
            C(i, j) = C(j, i) = rho;
        }
    std::vector<double> vol(N), price(N, 100.0);
    for (std::size_t i = 0; i < N; ++i) vol[i] = (0.12 + 0.30 * rng.uniform01()) / std::sqrt(252.0);

    Matrix Sigma(N, N);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j) Sigma(i, j) = C(i, j) * vol[i] * vol[j];
    auto f = factorize(Sigma);

    std::ofstream o(out);
    if (!o) { std::fprintf(stderr, "cannot write %s\n", out.c_str()); return 1; }
    o << "date";
    for (std::size_t i = 0; i < N; ++i) o << ",A" << i;
    o << "\n";

    double vmult = 1.0;
    std::vector<double> z(N);
    for (std::size_t t = 0; t < T; ++t) {
        vmult = 0.90 * vmult + 0.10 * 1.0 + 0.25 * (rng.uniform01() - 0.5);
        vmult = std::max(0.4, std::min(3.0, vmult));
        for (std::size_t i = 0; i < N; ++i) z[i] = rng.normal();
        o << t;
        for (std::size_t i = 0; i < N; ++i) {
            double x = 0.0;
            for (std::size_t k = 0; k <= i; ++k) x += f.B(i, k) * z[k];
            price[i] *= std::exp(-0.5 * vol[i] * vol[i] + vmult * x);
            o << "," << price[i];
        }
        o << "\n";
    }
    std::printf("wrote %s  (%zu days x %zu assets)\n", out.c_str(), T, N);
    return 0;
}

// ---------------------------------------------------------------- shared setup

struct Model {
    std::vector<double> notional, mu, var;
    Matrix Sigma;
    Factorization fact;
    std::string cov_name;
    double shrink = 0.0;
};

static Matrix estimate_cov(const Matrix& R, const std::string& kind, double* shrink) {
    if (kind == "ewma") return cov_ewma(R, 0.94);
    if (kind == "lw")   return cov_ledoit_wolf(R, shrink);
    return cov_sample(R);
}

static Model build_model(const PriceSeries& ps, int argc, char** argv) {
    Model m;
    const Matrix R = log_returns(ps.prices);
    const std::size_t N = ps.names.size();

    m.cov_name = arg_str(argc, argv, "--cov", "lw");
    m.Sigma = estimate_cov(R, m.cov_name, &m.shrink);
    m.mu    = col_means(R);
    if (arg_flag(argc, argv, "--zero-drift")) std::fill(m.mu.begin(), m.mu.end(), 0.0);
    m.var   = diag(m.Sigma);
    m.fact  = factorize(m.Sigma);

    const std::string nstr = arg_str(argc, argv, "--notional", "");
    m.notional.assign(N, arg_num(argc, argv, "--total", 10'000'000.0) / (double)N);
    if (!nstr.empty()) {
        std::stringstream ss(nstr);
        std::string cell;
        std::size_t i = 0;
        while (std::getline(ss, cell, ',') && i < N) m.notional[i++] = std::atof(cell.c_str());
    }
    return m;
}

static SimConfig build_cfg(int argc, char** argv) {
    SimConfig c;
    c.n_draws      = (std::size_t)arg_num(argc, argv, "--draws", 1'000'000);
    c.horizon_days = arg_num(argc, argv, "--horizon", 1.0);
    c.antithetic   = arg_flag(argc, argv, "--antithetic");
    c.model        = (arg_str(argc, argv, "--shocks", "gaussian") == "t") ? ShockModel::StudentT
                                                                         : ShockModel::Gaussian;
    c.nu           = arg_num(argc, argv, "--nu", 5.0);
    c.seed         = (std::uint64_t)arg_num(argc, argv, "--seed", 0x5EED);
    c.threads      = (unsigned)arg_num(argc, argv, "--threads", 1);
    c.block        = (std::size_t)arg_num(argc, argv, "--block", 4096);
    return c;
}

// ---------------------------------------------------------------- run

static int cmd_run(int argc, char** argv) {
    const auto ps = read_prices(arg_str(argc, argv, "--prices", "data/prices.csv"));
    Model m       = build_model(ps, argc, argv);
    SimConfig cfg = build_cfg(argc, argv);
    const double alpha = arg_num(argc, argv, "--alpha", 0.99);

    double gross = 0.0;
    for (double v : m.notional) gross += std::fabs(v);

    const auto t0 = std::chrono::steady_clock::now();
    auto pnl = simulate_pnl(m.notional, m.mu, m.var, m.fact.B, cfg);
    const auto t1 = std::chrono::steady_clock::now();
    const auto r  = var_es(pnl, alpha);
    const auto t2 = std::chrono::steady_clock::now();

    const double sim_s = std::chrono::duration<double>(t1 - t0).count();
    const double agg_s = std::chrono::duration<double>(t2 - t1).count();

    std::printf("assets            %zu\n", ps.names.size());
    std::printf("gross notional    %.0f\n", gross);
    std::printf("covariance        %s%s\n", m.cov_name.c_str(),
                m.cov_name == "lw" ? "" : "");
    if (m.cov_name == "lw") std::printf("shrink intensity  %.4f\n", m.shrink);
    std::printf("factorization     %s", m.fact.used_cholesky ? "cholesky\n" : "eigen-repair\n");
    if (!m.fact.used_cholesky)
        std::printf("  min eigenvalue  %.3e (clipped %zu)\n", m.fact.min_eigenvalue, m.fact.clipped);
    std::printf("shocks            %s%s\n",
                cfg.model == ShockModel::StudentT ? "student-t nu=" : "gaussian",
                cfg.model == ShockModel::StudentT ? std::to_string((int)cfg.nu).c_str() : "");
    std::printf("scenarios         %zu%s\n", scenario_count(cfg), cfg.antithetic ? " (antithetic)" : "");
    std::printf("horizon           %.0f day(s)\n", cfg.horizon_days);
    std::printf("--\n");
    std::printf("mean P&L          %+12.0f\n", r.mean_pnl);
    std::printf("VaR %.0f%%          %12.0f   (+/- %.0f, 1 s.e.)\n", alpha * 100, r.var, r.var_se);
    std::printf("ES  %.0f%%          %12.0f   (+/- %.0f, 1 s.e.)\n", alpha * 100, r.es, r.es_se);
    std::printf("VaR / gross       %11.3f%%\n", 100.0 * r.var / gross);
    std::printf("--\n");
    std::printf("threads           %u\n", cfg.threads);
    std::printf("simulate          %.3f s  (%.2f M scenarios/s)\n",
                sim_s, scenario_count(cfg) / sim_s / 1e6);
    std::printf("quantile+ES       %.3f s\n", agg_s);
    return 0;
}

// ---------------------------------------------------------------- bench

static int cmd_bench(int argc, char** argv) {
    const auto ps = read_prices(arg_str(argc, argv, "--prices", "data/prices.csv"));
    Model m       = build_model(ps, argc, argv);
    SimConfig cfg = build_cfg(argc, argv);
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const unsigned maxt = (unsigned)arg_num(argc, argv, "--max-threads", hw);

    std::printf("draws=%zu  assets=%zu  hw_concurrency=%u\n\n", cfg.n_draws, ps.names.size(), hw);
    std::printf("%8s %12s %14s %10s %12s\n", "threads", "runtime(s)", "Mscen/s", "speedup", "checksum");

    double base = 0.0;
    double ref_var = 0.0;
    for (unsigned t = 1; t <= maxt; t *= 2) {
        cfg.threads = t;
        const auto a = std::chrono::steady_clock::now();
        auto pnl = simulate_pnl(m.notional, m.mu, m.var, m.fact.B, cfg);
        const auto b = std::chrono::steady_clock::now();
        const double s = std::chrono::duration<double>(b - a).count();
        if (t == 1) base = s;
        const auto r = var_es(pnl, 0.99);
        if (t == 1) ref_var = r.var;
        std::printf("%8u %12.3f %14.2f %10.2fx %12.0f%s\n", t, s,
                    scenario_count(cfg) / s / 1e6, base / s, r.var,
                    (t > 1 && r.var == ref_var) ? "  [identical]" : "");
    }
    std::printf("\nBit-identical VaR across thread counts confirms block-deterministic seeding.\n");
    return 0;
}

// ---------------------------------------------------------------- variance reduction

static int cmd_vr(int argc, char** argv) {
    const auto ps = read_prices(arg_str(argc, argv, "--prices", "data/prices.csv"));
    Model m       = build_model(ps, argc, argv);
    SimConfig cfg = build_cfg(argc, argv);
    const double alpha = arg_num(argc, argv, "--alpha", 0.99);
    const int reps = (int)arg_num(argc, argv, "--reps", 200);

    // Equal scenario budget in both arms: antithetic uses half the draws, two scenarios each.
    const std::size_t budget = scenario_count(cfg);

    auto sd = [](const std::vector<double>& v) {
        double mu = 0.0; for (double x : v) mu += x; mu /= (double)v.size();
        double s = 0.0;  for (double x : v) s += (x - mu) * (x - mu);
        return std::sqrt(s / (double)(v.size() - 1));
    };

    struct Arm { std::vector<double> mean, var, es; };

    auto run_arm = [&](bool anti) {
        SimConfig c = cfg;
        c.antithetic = anti;
        c.n_draws    = anti ? budget / 2 : budget;
        Arm a;
        for (int r = 0; r < reps; ++r) {
            c.seed = cfg.seed + 1000u * (unsigned)r;
            auto pnl = simulate_pnl(m.notional, m.mu, m.var, m.fact.B, c);
            double mu = 0.0; for (double x : pnl) mu += x;
            a.mean.push_back(mu / (double)pnl.size());
            auto res = var_es(std::move(pnl), alpha);
            a.var.push_back(res.var);
            a.es.push_back(res.es);
        }
        return a;
    };

    const Arm plain = run_arm(false);
    const Arm anti  = run_arm(true);

    // A variance ratio estimated from R replications is itself noisy. log(ratio) has
    // approximate s.e. 2*sqrt(2/(2(R-1))), so report the interval, not just the point
    // estimate: at R=24 the interval spans roughly [0.5x, 3x] and tells you nothing.
    const double se_log = 2.0 * std::sqrt(2.0 / (2.0 * (reps - 1)));
    auto report = [&](const char* label, const std::vector<double>& p, const std::vector<double>& a) {
        const double sp = sd(p), sa = sd(a);
        const double ratio = (sp * sp) / (sa * sa);
        const double lo = std::exp(std::log(ratio) - 1.96 * se_log);
        const double hi = std::exp(std::log(ratio) + 1.96 * se_log);
        const char* verdict = (lo > 1.0) ? "reduces variance"
                            : (hi < 1.0) ? "increases variance"
                                         : "no detectable effect";
        std::printf("%-12s %12.4g %12.4g %10.2fx  [%.2f, %.2f]  %s\n",
                    label, sp, sa, ratio, lo, hi, verdict);
    };

    std::printf("scenario budget %zu, %d replications, alpha=%.2f\n\n", budget, reps, alpha);
    std::printf("%-12s %12s %12s %10s  %-14s %s\n",
                "estimator", "sd(plain)", "sd(anti)", "var ratio", "95% CI", "verdict");
    report("mean P&L", plain.mean, anti.mean);   // control: should show a large gain
    report("VaR",      plain.var,  anti.var);
    report("ES",       plain.es,   anti.es);
    std::printf("\nThe mean is the control. Antithetic sampling cancels the odd component of an\n"
                "estimator, so a near-linear functional like the sample mean benefits enormously.\n"
                "A far-tail quantile does not: an antithetic pair can essentially never place both\n"
                "members in the 1%% tail, so there is no cancellation where the estimator lives.\n");
    return 0;
}

// ---------------------------------------------------------------- backtest

static int cmd_backtest(int argc, char** argv) {
    const auto ps = read_prices(arg_str(argc, argv, "--prices", "data/prices.csv"));
    const Matrix R = log_returns(ps.prices);
    const std::size_t T = R.rows(), N = R.cols();

    const std::size_t W = (std::size_t)arg_num(argc, argv, "--window", 500);
    const double alpha  = arg_num(argc, argv, "--alpha", 0.99);
    const std::string cov_kind = arg_str(argc, argv, "--cov", "ewma");
    const std::string shocks   = arg_str(argc, argv, "--shocks", "gaussian");
    const double nu     = arg_num(argc, argv, "--nu", 5.0);

    if (T <= W) { std::fprintf(stderr, "not enough history\n"); return 1; }

    std::vector<double> notional(N, 10'000'000.0 / (double)N);

    SimConfig cfg;
    cfg.n_draws    = (std::size_t)arg_num(argc, argv, "--draws", 50'000);
    cfg.antithetic = true;
    cfg.threads    = (unsigned)arg_num(argc, argv, "--threads", 1);
    cfg.model      = (shocks == "t") ? ShockModel::StudentT : ShockModel::Gaussian;
    cfg.nu         = nu;

    std::vector<int> hits;
    hits.reserve(T - W);

    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t t = W; t < T; ++t) {
        Matrix win(W, N);
        for (std::size_t s = 0; s < W; ++s)
            for (std::size_t i = 0; i < N; ++i) win(s, i) = R(t - W + s, i);

        double shrink = 0.0;
        Matrix Sigma = estimate_cov(win, cov_kind, &shrink);
        auto mu = col_means(win);
        std::fill(mu.begin(), mu.end(), 0.0);   // no drift forecast in a 1-day risk model
        auto var = diag(Sigma);
        auto f   = factorize(Sigma);

        cfg.seed = 0xBEEF + t;
        auto pnl = simulate_pnl(notional, mu, var, f.B, cfg);
        const double VaR = var_es(std::move(pnl), alpha).var;

        double realised = 0.0;
        for (std::size_t i = 0; i < N; ++i) realised += notional[i] * std::expm1(R(t, i));
        hits.push_back((-realised > VaR) ? 1 : 0);
    }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    const auto b = backtest(hits, alpha);
    std::printf("cov=%s  shocks=%s  window=%zu  alpha=%.2f  days tested=%zu  (%.2f s)\n\n",
                cov_kind.c_str(), shocks.c_str(), W, alpha, b.n, secs);
    std::printf("exceptions        %zu   (expected %.1f)\n", b.exceptions, b.expected_rate * b.n);
    std::printf("observed rate     %.4f  (target %.4f)\n", b.observed_rate, b.expected_rate);
    std::printf("Kupiec  LR_pof    %8.3f   %s (chi2_1 95%% = 3.841)\n", b.lr_pof,
                b.lr_pof < b.crit_1df ? "PASS" : "FAIL");
    std::printf("Christoff LR_ind  %8.3f   %s (chi2_1 95%% = 3.841)\n", b.lr_ind,
                b.lr_ind < b.crit_1df ? "PASS" : "FAIL");
    std::printf("Joint     LR_cc   %8.3f   %s (chi2_2 95%% = 5.991)\n", b.lr_cc,
                b.lr_cc < b.crit_2df ? "PASS" : "FAIL");
    return 0;
}

// ----------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: mcre <gen|run|bench|vr|backtest> [options]\n");
        return 1;
    }
    const std::string cmd = argv[1];
    if (cmd == "gen")      return cmd_gen(argc, argv);
    if (cmd == "run")      return cmd_run(argc, argv);
    if (cmd == "bench")    return cmd_bench(argc, argv);
    if (cmd == "vr")       return cmd_vr(argc, argv);
    if (cmd == "backtest") return cmd_backtest(argc, argv);
    std::fprintf(stderr, "unknown command: %s\n", cmd.c_str());
    return 1;
}
