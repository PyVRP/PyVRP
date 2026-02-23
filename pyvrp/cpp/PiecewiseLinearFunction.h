#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pyvrp
{
/**
 * PiecewiseLinearFunction(
 *     breakpoints: list[int] = [min_int64, max_int64],
 *     segments: list[tuple[int, int]] = [(0, 0)],
 * )
 *
 * Creates a piecewise linear function.
 *
 * The given ``breakpoints`` define segment intervals. Segment ``i`` applies to
 * ``[breakpoints[i], breakpoints[i + 1])`` and uses
 * ``segments[i] = (intercept, slope)``:
 *
 * ``f(x) = intercept + slope * (x - breakpoints[i])``.
 *
 * The final interval is right-closed to include ``breakpoints[-1]``.
 *
 * Bounds behaviour
 * ----------------
 * This implementation throws a ``std::domain_error`` if ``x`` is outside the
 * function domain ``[breakpoints.front(), breakpoints.back()]``.
 *
 * Example:
 *
 * - ``breakpoints = [0, 5, 10]``
 * - ``segments = [(1, 2), (11, 3)]``
 *
 * Then:
 *
 * - ``f(2) = 1 + 2 * (2 - 0) = 5``
 * - ``f(7) = 11 + 3 * (7 - 5) = 17``
 *
 * Parameters
 * ----------
 * breakpoints
 *     Strictly increasing breakpoints. Must contain at least two elements.
 * segments
 *     Segment definitions ``(intercept, slope)`` for each interval. Must have
 *     size ``len(breakpoints) - 1``.
 */
template <typename Domain, typename Codomain> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Domain, Domain>;

private:
    std::vector<Domain> breakpoints_;
    std::vector<Segment> segments_;

public:
    PiecewiseLinearFunction();
    PiecewiseLinearFunction(std::vector<Domain> breakpoints,
                            std::vector<Segment> segments);

    /**
     * Evaluates the function at ``x``.
     */
    [[nodiscard]] inline Codomain operator()(Domain x) const;

    /**
     * Segment breakpoints of this function.
     */
    [[nodiscard]] std::vector<Domain> const &breakpoints() const;

    /**
     * Segment intercept/slope pairs.
     */
    [[nodiscard]] std::vector<Segment> const &segments() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

template <typename Domain, typename Codomain>
PiecewiseLinearFunction<Domain, Codomain>::PiecewiseLinearFunction()
    : PiecewiseLinearFunction({std::numeric_limits<Domain>::min(),
                               std::numeric_limits<Domain>::max()},
                              {{Domain{0}, Domain{0}}})
{
}

template <typename Domain, typename Codomain>
PiecewiseLinearFunction<Domain, Codomain>::PiecewiseLinearFunction(
    std::vector<Domain> breakpoints, std::vector<Segment> segments)
    : breakpoints_(std::move(breakpoints)), segments_(std::move(segments))
{
    if (breakpoints_.size() < 2)
    {
        auto const *msg = "breakpoints must contain at least two values.";
        throw std::invalid_argument(msg);
    }

    if (segments_.size() != breakpoints_.size() - 1)
    {
        auto const *msg = "segments must have size breakpoints.size() - 1.";
        throw std::invalid_argument(msg);
    }

    for (size_t idx = 1; idx != breakpoints_.size(); ++idx)
        if (breakpoints_[idx - 1] >= breakpoints_[idx])
            throw std::invalid_argument(
                "breakpoints must be strictly increasing.");
}

template <typename Domain, typename Codomain>
Codomain PiecewiseLinearFunction<Domain, Codomain>::operator()(Domain x) const
{
    if (x < breakpoints_.front() || x > breakpoints_.back())
        throw std::domain_error("x must be within function domain.");

    // Binary Search for the right segment. We can skip the first and last
    // breakpoint, as they are only used for bounds checking and not for segment
    // selection. Intervals are [b[i], b[i + 1]), except the final interval is
    // right-closed.
    auto const it
        = std::upper_bound(breakpoints_.begin() + 1, breakpoints_.end() - 1, x);
    auto const idx = static_cast<size_t>(it - breakpoints_.begin() - 1);

    auto const &[intercept, slope] = segments_[idx];
    if (slope == Domain{0})
        return static_cast<Codomain>(intercept);

    auto const delta = x - breakpoints_[idx];
    auto const value = intercept + slope * delta;
    return static_cast<Codomain>(value);
}

template <typename Domain, typename Codomain>
std::vector<Domain> const &
PiecewiseLinearFunction<Domain, Codomain>::breakpoints() const
{
    return breakpoints_;
}

template <typename Domain, typename Codomain>
std::vector<typename PiecewiseLinearFunction<Domain, Codomain>::Segment> const &
PiecewiseLinearFunction<Domain, Codomain>::segments() const
{
    return segments_;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
