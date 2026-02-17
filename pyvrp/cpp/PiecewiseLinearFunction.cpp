#include "PiecewiseLinearFunction.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

using pyvrp::PiecewiseLinearFunction;

namespace
{
// NOTE: #925/1044-FormPup41:
// These overflow helpers are currently local to this file so the checked
// arithmetic logic is explicit and compiler-independent. If more modules need
// the same checked int64 arithmetic, this can be moved to a shared internal
// utility.

// Returns true if lhs + rhs would overflow int64_t.
[[nodiscard]] bool addOverflows(
    PiecewiseLinearFunction::Scalar lhs,
    PiecewiseLinearFunction::Scalar rhs
)
{
    using Scalar = PiecewiseLinearFunction::Scalar;
    auto constexpr MAX = std::numeric_limits<Scalar>::max();
    auto constexpr MIN = std::numeric_limits<Scalar>::min();

    if (rhs > 0)
        return lhs > MAX - rhs;

    if (rhs < 0)
        return lhs < MIN - rhs;

    return false;
}

// Returns true if lhs * rhs would overflow int64_t.
[[nodiscard]] bool mulOverflows(
    PiecewiseLinearFunction::Scalar lhs,
    PiecewiseLinearFunction::Scalar rhs
)
{
    using Scalar = PiecewiseLinearFunction::Scalar;
    auto constexpr MAX = std::numeric_limits<Scalar>::max();
    auto constexpr MIN = std::numeric_limits<Scalar>::min();

    if (lhs == 0 || rhs == 0)
        return false;

    // This is the only product that cannot be represented in int64_t.
    if ((lhs == -1 && rhs == MIN) || (rhs == -1 && lhs == MIN))
        return true;

    if (lhs > 0)
    {
        if (rhs > 0)
            return lhs > MAX / rhs;

        return rhs < MIN / lhs;
    }

    // lhs < 0
    if (rhs > 0)
        return lhs < MIN / rhs;

    // lhs < 0 && rhs < 0
    return lhs < MAX / rhs;
}

[[nodiscard]] PiecewiseLinearFunction::Scalar checkedMulAdd(
    PiecewiseLinearFunction::Scalar lhs,
    PiecewiseLinearFunction::Scalar rhs,
    PiecewiseLinearFunction::Scalar addend
)
{
    // Computes addend + lhs * rhs with explicit overflow checks.
    if (mulOverflows(lhs, rhs))
    {
        auto const *msg = "PiecewiseLinearFunction multiplication overflow.";
        throw std::overflow_error(msg);
    }

    auto const product = lhs * rhs;

    if (addOverflows(addend, product))
    {
        auto const *msg = "PiecewiseLinearFunction addition overflow.";
        throw std::overflow_error(msg);
    }

    return addend + product;
}
}  // namespace

PiecewiseLinearFunction::PiecewiseLinearFunction(
    std::vector<Scalar> breakpoints,
    std::vector<Scalar> slopes,
    Scalar intercept
)
    : breakpoints_(std::move(breakpoints)),
      slopes_(std::move(slopes)),
      values_(breakpoints_.size(), 0),
      intercept_(intercept)
{
    if (breakpoints_.empty())
        throw std::invalid_argument("breakpoints must not be empty.");

    if (breakpoints_.size() != slopes_.size())
    {
        auto const *msg = "breakpoints and slopes must have equal length.";
        throw std::invalid_argument(msg);
    }

    if (!std::is_sorted(breakpoints_.begin(), breakpoints_.end()))
        throw std::invalid_argument("breakpoints must be sorted.");

    for (size_t idx = 1; idx != breakpoints_.size(); ++idx)
        if (breakpoints_[idx - 1] >= breakpoints_[idx])
            throw std::invalid_argument("breakpoints must be strictly increasing.");

    // values_[i] stores f(breakpoints_[i]). We compute these cumulatively so
    // evaluating f(x) later only needs one segment lookup and one linear step.
    values_[0] = intercept_;
    for (size_t idx = 1; idx != breakpoints_.size(); ++idx)
    {
        auto const delta = breakpoints_[idx] - breakpoints_[idx - 1];

        // f(b_i) = f(b_{i-1}) + slope_{i-1} * (b_i - b_{i-1}).
        values_[idx] = checkedMulAdd(slopes_[idx - 1], delta, values_[idx - 1]);
    }
}

PiecewiseLinearFunction::Scalar PiecewiseLinearFunction::operator()(Scalar x) const
{
    if (x < breakpoints_.front())
    {
        // NOTE: #925/1044 - FormPup41:
        // Instead of throwing an exception, we could also include an argument
        // in the constructor to specify behavior for out-of-domain inputs, for
        // example extrapolate with the first slope or clamp to the first
        // breakpoint, adding more flexibility.
        // For now, we keep it strict to avoid confusion about the function domain.
        auto const *msg = "x must be >= first breakpoint.";
        throw std::invalid_argument(msg);
    }

    auto const ub = std::upper_bound(breakpoints_.begin(), breakpoints_.end(), x);
    auto const idx = static_cast<size_t>(std::distance(breakpoints_.begin(), ub)) - 1;

    // x lies in segment idx, so reconstruct f(x) from the precomputed value at
    // the segment start.
    auto const delta = x - breakpoints_[idx];
    return checkedMulAdd(slopes_[idx], delta, values_[idx]);
}

bool PiecewiseLinearFunction::isZero() const
{
    if (intercept_ != 0)
        return false;

    return std::all_of(
        slopes_.begin(), slopes_.end(), [](auto const slope) { return slope == 0; });
}
