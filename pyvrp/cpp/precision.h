#ifndef PRECISION_H
#define PRECISION_H

#include <cmath>

/**
 * PyVRP can be compiled to support two types of distance and duration types:
 * integer or double. Double precision is default, and required by e.g. SINTEF
 * for their benchmarks. Integer precision is also possible, and is somewhat
 * faster. It can be used for e.g. the DIMACS benchmarks.
 */
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

#endif  // PRECISION_H
