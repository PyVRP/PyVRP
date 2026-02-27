#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <cassert>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace pyvrp
{
/**
 * PiecewiseLinearFunction(
 *     points: list[tuple[np.int64, np.int64]
 *                  | tuple[np.int64, np.int64, np.int64]],
 * )
 *
 * Creates a piecewise linear function :math:`f` from ordered
 * ``(breakpoint, slope[, jump])`` points.
 *
 * Points are required to be strictly increasing in breakpoint. Each slope
 * applies from that breakpoint onward. The optional ``jump`` value specifies
 * an additional fixed cost that is applied at that breakpoint and to the
 * right. When omitted, ``jump`` defaults to zero.
 *
 * The first point also defines extrapolation to the left. In particular, for
 * a first point ``(x_0, s_0, z_0)``, we have ``f(x_0) = z_0`` and slope
 * ``s_0`` for :math:`x < x_0`.
 *
 * Internally, the function is represented via :math:`n` breakpoints and linear
 * segments :math:`(b_i, f_i)_{i = 1, \ldots, n}`. Given :math:`x`, define
 *
 * .. math::
 *    f(x) =
 *    \begin{cases}
 *       f_1(x) & \text{ if } x < b_1, \\
 *       f_i(x) & \text{ if } b_{i - 1} \le x < b_i
 *                \text{ for } i = 2, \ldots, n, \\
 *       f_n(x) & \text{ otherwise.}
 *    \end{cases}
 *
 * Parameters
 * ----------
 * points
 *     Ordered ``(breakpoint, slope[, jump])`` points defining the function.
 */
template <typename Dom, typename Co> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Dom, Dom>;
    struct Point
    {
        Dom breakpoint;
        Dom slope;
        Dom jump;

        bool operator==(Point const &other) const = default;
    };

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;

    [[nodiscard]] static Dom safeAdd(Dom lhs, Dom rhs, char const *message);
    [[nodiscard]] static Dom
    safeSubtract(Dom lhs, Dom rhs, char const *message);
    [[nodiscard]] static Dom
    safeMultiply(Dom lhs, Dom rhs, char const *message);

public:
    PiecewiseLinearFunction();

    PiecewiseLinearFunction(std::vector<Point> points);

    PiecewiseLinearFunction(std::vector<Dom> breakpoints,
                            std::vector<Segment> segments);

    /**
     * Evaluates :math:`f(x)`.
     */
    [[nodiscard]] inline Co operator()(Dom x) const;

    /**
     * Breakpoints.
     */
    [[nodiscard]] std::vector<Dom> const &breakpoints() const;

    /**
     * Segment (intercept, slope) coefficients.
     */
    [[nodiscard]] std::vector<Segment> const &segments() const;

    /**
     * Returns whether this function is monotonically increasing.
     */
    [[nodiscard]] bool isMonotonicallyIncreasing() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction()
    : PiecewiseLinearFunction({std::numeric_limits<Dom>::min()}, {{0, 0}})
{
}

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Point> points)
{
    static_assert(std::is_integral_v<Dom>,
                  "PiecewiseLinearFunction expects an integral domain type.");

    if (points.empty())
        throw std::invalid_argument("Need at least one point.");

    for (size_t idx = 0; idx + 1 != points.size(); ++idx)
        if (points[idx].breakpoint >= points[idx + 1].breakpoint)
            throw std::invalid_argument(
                "Points must be strictly increasing in breakpoint.");

    std::vector<Dom> values(points.size());
    values[0] = points[0].jump;  // f(x_0) equals the first jump value.

    for (size_t idx = 1; idx != points.size(); ++idx)
    {
        auto const dx = safeSubtract(points[idx].breakpoint,
                                     points[idx - 1].breakpoint,
                                     "Point coordinates differ by too much.");
        auto const delta = safeMultiply(
            points[idx - 1].slope, dx, "Point coordinates result in overflow.");
        auto const leftValue = safeAdd(
            values[idx - 1], delta, "Point coordinates result in overflow.");

        values[idx] = safeAdd(leftValue,
                              points[idx].jump,
                              "Point coordinates result in overflow.");
    }

    breakpoints_.reserve(points.size() + 1);
    for (auto const &point : points)
        breakpoints_.push_back(point.breakpoint);

    // Duplicate the final breakpoint so jumps at the last point are modeled.
    breakpoints_.push_back(points.back().breakpoint);

    auto const segmentFromAnchor = [&](size_t idx)
    {
        auto const product
            = safeMultiply(points[idx].slope,
                           points[idx].breakpoint,
                           "Point coordinates result in overflow.");
        auto const intercept = safeSubtract(
            values[idx], product, "Point coordinates result in overflow.");
        return Segment{intercept, points[idx].slope};
    };

    segments_.reserve(points.size() + 1);
    segments_.push_back(segmentFromAnchor(0));  // left-side extrapolation

    for (size_t idx = 0; idx + 1 != points.size(); ++idx)
        segments_.push_back(segmentFromAnchor(idx));

    segments_.push_back(segmentFromAnchor(points.size() - 1));
}

template <typename Dom, typename Co>
Dom PiecewiseLinearFunction<Dom, Co>::safeAdd(Dom lhs,
                                              Dom rhs,
                                              char const *message)
{
    Dom result = 0;
    if (__builtin_add_overflow(lhs, rhs, &result))
        throw std::invalid_argument(message);
    return result;
}

template <typename Dom, typename Co>
Dom PiecewiseLinearFunction<Dom, Co>::safeSubtract(Dom lhs,
                                                   Dom rhs,
                                                   char const *message)
{
    Dom result = 0;
    if (__builtin_sub_overflow(lhs, rhs, &result))
        throw std::invalid_argument(message);
    return result;
}

template <typename Dom, typename Co>
Dom PiecewiseLinearFunction<Dom, Co>::safeMultiply(Dom lhs,
                                                   Dom rhs,
                                                   char const *message)
{
    Dom result = 0;
    if (__builtin_mul_overflow(lhs, rhs, &result))
        throw std::invalid_argument(message);
    return result;
}

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Dom> breakpoints, std::vector<Segment> segments)
    : breakpoints_(std::move(breakpoints)), segments_(std::move(segments))
{
    if (segments_.empty())
        throw std::invalid_argument("Need at least one segment.");

    if (breakpoints_.size() != segments_.size())
        throw std::invalid_argument(
            "Number of breakpoints and segments must match.");

    for (size_t idx = 0; idx != breakpoints_.size() - 1; ++idx)
        if (breakpoints_[idx] > breakpoints_[idx + 1])
            throw std::invalid_argument("Breakpoints must be non-decreasing.");
}

template <typename Dom, typename Co>
Co PiecewiseLinearFunction<Dom, Co>::operator()(Dom x) const
{
    for (size_t idx = 0; idx != breakpoints_.size(); ++idx)
        if (x < breakpoints_[idx])
        {
            auto const [intercept, slope] = segments_[idx];
            return static_cast<Co>(intercept + slope * x);
        }

    auto const [intercept, slope] = segments_.back();
    return static_cast<Co>(intercept + slope * x);
}

template <typename Dom, typename Co>
std::vector<Dom> const &PiecewiseLinearFunction<Dom, Co>::breakpoints() const
{
    return breakpoints_;
}

template <typename Dom, typename Co>
std::vector<typename PiecewiseLinearFunction<Dom, Co>::Segment> const &
PiecewiseLinearFunction<Dom, Co>::segments() const
{
    return segments_;
}

template <typename Dom, typename Co>
bool PiecewiseLinearFunction<Dom, Co>::isMonotonicallyIncreasing() const
{
    for (auto const &segment : segments_)
    {
        auto const slope = segment.second;
        if (slope < 0)
            return false;
    }

    for (size_t idx = 1; idx != segments_.size(); ++idx)
    {
        auto const breakpoint = static_cast<long double>(breakpoints_[idx - 1]);
        auto const [prevIntercept, prevSlope] = segments_[idx - 1];
        auto const [nextIntercept, nextSlope] = segments_[idx];

        auto const left = static_cast<long double>(prevIntercept)
                          + static_cast<long double>(prevSlope) * breakpoint;
        auto const right = static_cast<long double>(nextIntercept)
                           + static_cast<long double>(nextSlope) * breakpoint;

        if (right < left)
            return false;
    }

    return true;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
