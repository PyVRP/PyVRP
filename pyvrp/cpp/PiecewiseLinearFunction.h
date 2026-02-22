#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
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
template <typename Domain = int64_t, typename Codomain = int64_t>
class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Codomain, Codomain>;

private:
    std::vector<Domain> breakpoints_;
    std::vector<Segment> segments_;

    template <typename Value>
        requires requires(Value const value) { value.get(); }
    [[nodiscard]] static auto toRaw(Value const value);

    template <typename Value>
        requires(!requires(Value const value) { value.get(); })
    [[nodiscard]] static Value toRaw(Value const value);

    template <typename Value, typename Raw>
        requires std::is_arithmetic_v<Value>
    [[nodiscard]] static Value fromRaw(Raw const value);

    template <typename Value, typename Raw>
        requires(!std::is_arithmetic_v<Value>)
    [[nodiscard]] static Value fromRaw(Raw const value);

public:
    PiecewiseLinearFunction();
    PiecewiseLinearFunction(std::vector<Domain> breakpoints,
                            std::vector<Segment> segments);

    /**
     * Evaluates the function at ``x``.
     */
    [[nodiscard]] Codomain operator()(Domain x) const;

    /**
     * Segment breakpoints of this function.
     */
    [[nodiscard]] std::vector<Domain> const &breakpoints() const;

    /**
     * Segment intercept/slope pairs.
     */
    [[nodiscard]] std::vector<Segment> const &segments() const;

    /**
     * Tests whether this function is the zero function.
     */
    [[nodiscard]] bool isZero() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

template <typename Domain, typename Codomain>
template <typename Value>
    requires requires(Value const value) { value.get(); }
auto PiecewiseLinearFunction<Domain, Codomain>::toRaw(Value const value)
{
    return value.get();
}

template <typename Domain, typename Codomain>
template <typename Value>
    requires(!requires(Value const value) { value.get(); })
Value PiecewiseLinearFunction<Domain, Codomain>::toRaw(Value const value)
{
    return value;
}

template <typename Domain, typename Codomain>
template <typename Value, typename Raw>
    requires std::is_arithmetic_v<Value>
Value PiecewiseLinearFunction<Domain, Codomain>::fromRaw(Raw const value)
{
    return static_cast<Value>(value);
}

template <typename Domain, typename Codomain>
template <typename Value, typename Raw>
    requires(!std::is_arithmetic_v<Value>)
Value PiecewiseLinearFunction<Domain, Codomain>::fromRaw(Raw const value)
{
    return Value(value);
}

template <typename Domain, typename Codomain>
PiecewiseLinearFunction<Domain, Codomain>::PiecewiseLinearFunction()
    : PiecewiseLinearFunction({std::numeric_limits<Domain>::min(),
                               std::numeric_limits<Domain>::max()},
                              {{fromRaw<Codomain>(0), fromRaw<Codomain>(0)}})
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

    if (segments_.size() + 1 != breakpoints_.size())
    {
        auto const *msg = "segments must have size breakpoints.size() - 1.";
        throw std::invalid_argument(msg);
    }

    for (size_t idx = 1; idx != breakpoints_.size(); ++idx)
        if (!(breakpoints_[idx - 1] < breakpoints_[idx]))
            throw std::invalid_argument(
                "breakpoints must be strictly increasing.");
}

template <typename Domain, typename Codomain>
Codomain PiecewiseLinearFunction<Domain, Codomain>::operator()(Domain x) const
{
    if (x < breakpoints_.front() || x > breakpoints_.back())
        throw std::domain_error("x must be within function domain.");

    size_t idx = 0;
    for (; idx + 1 != breakpoints_.size(); ++idx)
        if (x < breakpoints_[idx + 1])
            break;

    if (idx == segments_.size())  // x == breakpoints_.back()
        idx = segments_.size() - 1;

    auto const &[intercept, slope] = segments_[idx];
    auto const delta = toRaw(x - breakpoints_[idx]);
    auto const value = toRaw(intercept) + toRaw(slope) * delta;
    return fromRaw<Codomain>(value);
}

template <typename Domain, typename Codomain>
inline std::vector<Domain> const &
PiecewiseLinearFunction<Domain, Codomain>::breakpoints() const
{
    return breakpoints_;
}

template <typename Domain, typename Codomain>
inline auto PiecewiseLinearFunction<Domain, Codomain>::segments() const
    -> std::vector<Segment> const &
{
    return segments_;
}

template <typename Domain, typename Codomain>
bool PiecewiseLinearFunction<Domain, Codomain>::isZero() const
{
    auto const zero = fromRaw<Codomain>(0);
    return std::all_of(segments_.begin(),
                       segments_.end(),
                       [&](Segment const &segment) {
                           return segment.first == zero
                                  && segment.second == zero;
                       });
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
