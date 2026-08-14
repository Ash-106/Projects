#pragma once
#include <cstdint>
#include <cmath>

namespace mcre {

inline std::uint64_t splitmix64(std::uint64_t& x) {
    std::uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// xoshiro256++. Chosen over std::mt19937_64 for state size (32B vs 2.5KB, so it stays
// in L1 alongside the working set) and ~3x throughput in the inner loop.
class Rng {
public:
    explicit Rng(std::uint64_t seed) {
        std::uint64_t x = seed;
        for (int i = 0; i < 4; ++i) s_[i] = splitmix64(x);
    }

    std::uint64_t next_u64() {
        const std::uint64_t r = rotl(s_[0] + s_[3], 23) + s_[0];
        const std::uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return r;
    }

    double uniform01() { return static_cast<double>(next_u64() >> 11) * 0x1.0p-53; }

    // Marsaglia polar method: no trig, one cached spare per pair.
    double normal() {
        if (has_cached_) { has_cached_ = false; return cached_; }
        double u, v, s;
        do {
            u = 2.0 * uniform01() - 1.0;
            v = 2.0 * uniform01() - 1.0;
            s = u * u + v * v;
        } while (s >= 1.0 || s == 0.0);
        const double f = std::sqrt(-2.0 * std::log(s) / s);
        cached_ = v * f;
        has_cached_ = true;
        return u * f;
    }

    // Marsaglia-Tsang gamma sampler.
    double gamma(double a) {
        if (a < 1.0) return gamma(a + 1.0) * std::pow(uniform01(), 1.0 / a);
        const double d = a - 1.0 / 3.0;
        const double c = 1.0 / std::sqrt(9.0 * d);
        for (;;) {
            double x = normal();
            double v = 1.0 + c * x;
            if (v <= 0.0) continue;
            v = v * v * v;
            const double u = uniform01();
            if (u < 1.0 - 0.0331 * x * x * x * x) return d * v;
            if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v))) return d * v;
        }
    }

    double chi2(double nu) { return 2.0 * gamma(0.5 * nu); }

    void clear_cache() { has_cached_ = false; }

private:
    static std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    std::uint64_t s_[4];
    double cached_ = 0.0;
    bool   has_cached_ = false;
};

} // namespace mcre
