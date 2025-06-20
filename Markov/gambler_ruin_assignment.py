
from functools import lru_cache
from fractions import Fraction
import numpy as np

def gambler_win_probability(p, k, N):
    q = 1 - p
    if p == q:
        return k / N
    else:
        return (1 - (q / p) ** k) / (1 - (q / p) ** N)

def infinite_wealth_probability(p, k):
    q = 1 - p
    if p <= q:
        return 0
    else:
        return 1 - (q / p) ** k

def expected_rounds(p, k, N):
    q = 1 - p
    if p == q:
        return k * (N - k)
    else:
        r = q / p
        numerator = (1 - r ** k) * (1 - r ** (N - k))
        denominator = (1 - r) ** 2 * (1 - r ** N)
        return numerator / denominator

def aggressive_win_probability(p, k, N):
    q = 1 - p

    @lru_cache(None)
    def win_prob(k):
        if k == 0:
            return 0
        if k == N:
            return 1
        if k < N // 2:
            return p * win_prob(2 * k)
        else:
            return p + q * win_prob(2 * k - N)
    
    return win_prob(k)

def aggressive_expected_rounds(p, k, N):
    q = 1 - p

    @lru_cache(None)
    def expected_time(k):
        if k == 0 or k == N:
            return 0
        if k < N // 2:
            return 1 + p * expected_time(2 * k) + q * expected_time(0)
        else:
            return 1 + p * expected_time(N) + q * expected_time(2 * k - N)

    return expected_time(k)

def mod_inverse_game(p, q, k, t, m, W, M=10**9 + 7):
    dp = [Fraction(0) for _ in range(k + 2)]

    for i in reversed(range(t, k + 1)):
        if i <= m + W - 1:
            dp[i] = 1 + p * dp[min(k, i + 1)] + q * dp[i - 1]
        elif i == m + W:
            dp[i] = 1 + q * dp[i - 1]

    result = dp[k]
    a, b = result.numerator, result.denominator
    return (a * pow(b, -1, M)) % M

def stationary_distribution(p_list, r_list, q_list):
    N = len(p_list) - 1
    A = np.zeros((N + 1, N + 1))
    for i in range(N + 1):
        if i > 0:
            A[i][i - 1] = q_list[i]
        A[i][i] = r_list[i] - 1
        if i < N:
            A[i][i + 1] = p_list[i]
    A[0][0] = 1
    A[0][1:] = 0
    b = np.zeros(N + 1)
    b[0] = 1
    pi = np.linalg.solve(A, b)
    expected_price = sum(i * pi[i] for i in range(N + 1))
    return pi, expected_price

def expected_hitting_time(p_list, r_list, q_list, a, b):
    if a == b:
        return 0
    N = len(p_list) - 1
    h = [0.0] * (b + 1)
    for i in range(b - 1, a - 1, -1):
        p = p_list[i]
        r = r_list[i]
        q = q_list[i]
        h[i] = (1 + p * h[i + 1] + r * h[i] + q * h[i - 1]) / (1 - r)
    return h[a]
