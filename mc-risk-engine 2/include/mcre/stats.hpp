#pragma once
#include "matrix.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace mcre {

// prices: T x N. Returns (T-1) x N log returns.
inline Matrix log_returns(const Matrix& prices) {
    const std::size_t T = prices.rows(), N = prices.cols();
    Matrix R(T - 1, N);
    for (std::size_t t = 1; t < T; ++t)
        for (std::size_t i = 0; i < N; ++i)
            R(t - 1, i) = std::log(prices(t, i) / prices(t - 1, i));
    return R;
}

inline std::vector<double> col_means(const Matrix& R) {
    const std::size_t T = R.rows(), N = R.cols();
    std::vector<double> m(N, 0.0);
    for (std::size_t t = 0; t < T; ++t)
        for (std::size_t i = 0; i < N; ++i) m[i] += R(t, i);
    for (auto& v : m) v /= static_cast<double>(T);
    return m;
}

// Unbiased sample covariance, 1/(T-1).
inline Matrix cov_sample(const Matrix& R) {
    const std::size_t T = R.rows(), N = R.cols();
    const auto mu = col_means(R);
    Matrix S(N, N);
    for (std::size_t t = 0; t < T; ++t)
        for (std::size_t i = 0; i < N; ++i) {
            const double xi = R(t, i) - mu[i];
            for (std::size_t j = 0; j <= i; ++j) S(i, j) += xi * (R(t, j) - mu[j]);
        }
    const double d = static_cast<double>(T - 1);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j <= i; ++j) { S(i, j) /= d; S(j, i) = S(i, j); }
    return S;
}

// RiskMetrics-style EWMA, zero-mean convention. Weights recent observations more, so it
// tracks volatility clustering that an equal-weighted sample covariance smears out.
inline Matrix cov_ewma(const Matrix& R, double lambda = 0.94) {
    const std::size_t T = R.rows(), N = R.cols();
    Matrix S(N, N);
    double wsum = 0.0;
    for (std::size_t t = 0; t < T; ++t) {
        const double w = std::pow(lambda, static_cast<double>(T - 1 - t));
        wsum += w;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j <= i; ++j) S(i, j) += w * R(t, i) * R(t, j);
    }
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j <= i; ++j) { S(i, j) /= wsum; S(j, i) = S(i, j); }
    return S;
}

// Ledoit-Wolf shrinkage toward a scaled identity target.
// Sample covariance is a poor estimator when T/N is small: its extreme eigenvalues are
// biased outward, which systematically understates portfolio risk in the directions the
// optimiser/portfolio actually loads on. Shrinkage trades bias for a large variance
// reduction and guarantees a positive-definite estimate.
inline Matrix cov_ledoit_wolf(const Matrix& R, double* out_intensity = nullptr) {
    const std::size_t T = R.rows(), N = R.cols();
    const auto mu = col_means(R);

    Matrix X(T, N);
    for (std::size_t t = 0; t < T; ++t)
        for (std::size_t i = 0; i < N; ++i) X(t, i) = R(t, i) - mu[i];

    Matrix S(N, N);
    for (std::size_t t = 0; t < T; ++t)
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j <= i; ++j) S(i, j) += X(t, i) * X(t, j);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j <= i; ++j) {
            S(i, j) /= static_cast<double>(T);
            S(j, i) = S(i, j);
        }

    double trace = 0.0;
    for (std::size_t i = 0; i < N; ++i) trace += S(i, i);
    const double m = trace / static_cast<double>(N);

    double d2 = 0.0;
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j) {
            const double diff = S(i, j) - (i == j ? m : 0.0);
            d2 += diff * diff;
        }
    d2 /= static_cast<double>(N);

    double b2bar = 0.0;
    for (std::size_t t = 0; t < T; ++t) {
        double acc = 0.0;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j) {
                const double diff = X(t, i) * X(t, j) - S(i, j);
                acc += diff * diff;
            }
        b2bar += acc / static_cast<double>(N);
    }
    b2bar /= static_cast<double>(T) * static_cast<double>(T);

    const double b2 = std::min(b2bar, d2);
    const double shrink = (d2 > 0.0) ? b2 / d2 : 0.0;
    if (out_intensity) *out_intensity = shrink;

    Matrix Sig(N, N);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            Sig(i, j) = shrink * (i == j ? m : 0.0) + (1.0 - shrink) * S(i, j);
    return Sig;
}

inline std::vector<double> diag(const Matrix& A) {
    std::vector<double> d(A.rows());
    for (std::size_t i = 0; i < A.rows(); ++i) d[i] = A(i, i);
    return d;
}

} // namespace mcre
