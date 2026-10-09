#ifndef PYVRP_CONSTANTS_H
#define PYVRP_CONSTANTS_H

#include <cstddef>
#include <limits>

namespace pyvrp
{
// Maximum unsigned size on this platform.
inline constexpr std::size_t MAX_SIZE = std::numeric_limits<std::size_t>::max();

// Largest recommended input distance or duration matrix value, including
// missing values. Larger values may cause numerical instability.
inline constexpr double MAX_VALUE = 1e9;

// Absolute tolerance for feasibility and improvement decisions. Apply this at
// the comparison site, so intermediate calculations retain their precision.
inline constexpr double TOL = 1e-5;
}  // namespace pyvrp

#endif  // PYVRP_CONSTANTS_H
