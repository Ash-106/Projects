#pragma once
#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace mcre {

class Matrix {
public:
    Matrix() = default;
    Matrix(std::size_t n, std::size_t m, double v = 0.0) : n_(n), m_(m), d_(n * m, v) {}

    static Matrix identity(std::size_t n) {
        Matrix A(n, n);
        for (std::size_t i = 0; i < n; ++i) A(i, i) = 1.0;
        return A;
    }

    double&       operator()(std::size_t i, std::size_t j)       { return d_[i * m_ + j]; }
    const double& operator()(std::size_t i, std::size_t j) const { return d_[i * m_ + j]; }

    std::size_t rows() const { return n_; }
    std::size_t cols() const { return m_; }

private:
    std::size_t n_ = 0, m_ = 0;
    std::vector<double> d_;
};

// A = L L^T with L lower triangular. Returns false if A is not numerically PD.
// The pivot test is relative to the largest diagonal entry: accepting a pivot of ~1e-18
// on a matrix scaled at 1e-2 "succeeds" while producing a factor whose columns are pure
// rounding noise, which then shows up as nonsense correlations in the simulated shocks.
inline bool cholesky(const Matrix& A, Matrix& L, double tol = 1e-12) {
    const std::size_t n = A.rows();
    double scale = 0.0;
    for (std::size_t i = 0; i < n; ++i) scale = std::max(scale, std::fabs(A(i, i)));
    const double min_pivot = tol * scale;

    L = Matrix(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j <= i; ++j) {
            double s = A(i, j);
            for (std::size_t k = 0; k < j; ++k) s -= L(i, k) * L(j, k);
            if (i == j) {
                if (s <= min_pivot) return false;
                L(i, i) = std::sqrt(s);
            } else {
                L(i, j) = s / L(j, j);
            }
        }
    }
    return true;
}

// Cyclic Jacobi eigendecomposition for symmetric A. A = V diag(ev) V^T.
inline void jacobi_eigen(Matrix A, std::vector<double>& ev, Matrix& V, int sweeps = 100) {
    const std::size_t n = A.rows();
    V = Matrix::identity(n);
    for (int sweep = 0; sweep < sweeps; ++sweep) {
        double off = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = i + 1; j < n; ++j) off += A(i, j) * A(i, j);
        if (off < 1e-30) break;

        for (std::size_t p = 0; p < n; ++p) {
            for (std::size_t q = p + 1; q < n; ++q) {
                if (std::fabs(A(p, q)) < 1e-300) continue;
                const double theta = (A(q, q) - A(p, p)) / (2.0 * A(p, q));
                const double sgn   = (theta >= 0.0) ? 1.0 : -1.0;
                const double t     = sgn / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                const double c     = 1.0 / std::sqrt(t * t + 1.0);
                const double s     = t * c;

                for (std::size_t k = 0; k < n; ++k) {
                    const double apk = A(p, k), aqk = A(q, k);
                    A(p, k) = c * apk - s * aqk;
                    A(q, k) = s * apk + c * aqk;
                }
                for (std::size_t k = 0; k < n; ++k) {
                    const double akp = A(k, p), akq = A(k, q);
                    A(k, p) = c * akp - s * akq;
                    A(k, q) = s * akp + c * akq;
                }
                for (std::size_t k = 0; k < n; ++k) {
                    const double vkp = V(k, p), vkq = V(k, q);
                    V(k, p) = c * vkp - s * vkq;
                    V(k, q) = s * vkp + c * vkq;
                }
            }
        }
    }
    ev.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) ev[i] = A(i, i);
}

struct Factorization {
    Matrix B;            // X = B Z has covariance Sigma (approximately, if repaired)
    bool   used_cholesky = false;
    double min_eigenvalue = 0.0;   // only filled when the eigen path is taken
    std::size_t clipped = 0;       // number of non-positive eigenvalues clipped
};

// Preferred path: Cholesky (O(n^3/3), triangular B => cheaper matvec in the hot loop).
// Fallback: eigenvalue clipping, which yields a valid but dense square root. This matters
// because a sample covariance matrix estimated from T < N observations is singular, and
// short-window / EWMA estimates are frequently near-singular.
inline Factorization factorize(const Matrix& Sigma, double eps = 1e-14) {
    Factorization f;
    if (cholesky(Sigma, f.B)) { f.used_cholesky = true; return f; }

    std::vector<double> ev;
    Matrix V;
    jacobi_eigen(Sigma, ev, V);
    f.min_eigenvalue = *std::min_element(ev.begin(), ev.end());

    const std::size_t n = Sigma.rows();
    double scale = 0.0;
    for (double e : ev) scale = std::max(scale, std::fabs(e));
    const double floor_ = std::max(eps, scale * 1e-12);

    f.B = Matrix(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            double e = ev[j];
            if (e < floor_) e = floor_;
            f.B(i, j) = V(i, j) * std::sqrt(e);
        }
    }
    for (double e : ev) if (e < floor_) ++f.clipped;
    return f;
}

} // namespace mcre
