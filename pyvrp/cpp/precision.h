#ifndef PRECISION_H
#define PRECISION_H

#include <cmath>

#ifdef INT_PRECISION
using cost_type = int;
using distance_type = int;
using duration_type = int;
#else
using cost_type = double;
using distance_type = double;
using duration_type = double;
#endif

/**
 * Quick check whether a == b with a given tolerance. Exact when the types are
 * integral, approximate for floating point values.
 */
template <typename T>
[[nodiscard]] inline bool equal(T a, T b, double tol = 1e-6)
{
    if constexpr (std::is_integral_v<T>)
        return a == b;
    else
        return std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) * tol;
}

/**
 * Quick check whether a > b with a given tolerance. Exact when the types are
 * integral, approximate for floating point values.
 */
template <typename T>
[[nodiscard]] inline bool greater(T a, T b, double tol = 1e-6)
{
    if constexpr (std::is_integral_v<T>)
        return a > b;
    else
        return a - b > std::max(std::abs(a), std::abs(b)) * tol;
}

/**
 * Quick check whether a >= b with a given tolerance. Exact when the types are
 * integral, approximate for floating point values.
 */
template <typename T>
[[nodiscard]] inline bool greater_equal(T a, T b, double tol = 1e-6)
{
    if constexpr (std::is_integral_v<T>)
        return a >= b;
    else
        return greater(a, b, tol) || equal(a, b, tol);
}

/**
 * Quick check whether a < b with a given tolerance. Exact when the types are
 * integral, approximate for floating point values.
 */
template <typename T>
[[nodiscard]] inline bool less(T a, T b, double tol = 1e-6)
{
    if constexpr (std::is_integral_v<T>)
        return b > a;
    else
        return b - a > std::max(std::abs(a), std::abs(b)) * tol;
}

/**
 * Quick check whether a <= b with a given tolerance. Exact when the types are
 * integral, approximate for floating point values.
 */
template <typename T>
[[nodiscard]] inline bool less_equal(T a, T b, double tol = 1e-6)
{
    if constexpr (std::is_integral_v<T>)
        return b >= a;
    else
        return less(a, b, tol) || equal(a, b, tol);
}

#endif  // PRECISION_H
