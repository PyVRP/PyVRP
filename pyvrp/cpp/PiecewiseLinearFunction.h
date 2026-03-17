#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace pyvrp
{
/**
 * PiecewiseLinearFunction
 *
 * Represents a piecewise linear function :math:`f`.
 *
 * Internally, the function is represented via :math:`n` strictly increasing
 * breakpoints :math:`(b_i)_{i = 0, \ldots, n - 1}` and :math:`n + 1` linear
 * segments :math:`(f_i)_{i = 0, \ldots, n}`. Given :math:`x`, define
 *
 * .. math::
 *    f(x) =
 *    \begin{cases}
 *       f_0(x) & \text{ if } x < b_0, \\
 *       f_i(x) & \text{ if } b_{i - 1} \le x < b_i
 *                \text{ for } i = 1, \ldots, n - 1, \\
 *       f_n(x) & \text{ otherwise.}
 *    \end{cases}
 *
 */
template <typename Dom, typename Co> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Dom, Dom>;
    using Point = std::pair<Dom, Co>;

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;

public:
    /**
     * PiecewiseLinearFunction(
     *     points: list[tuple[np.int64, np.int64]],
     * )
     *
     * Creates a piecewise linear function from ordered ``(x, f(x))`` points.
     *
     * Points must be non-decreasing in ``x``. Jumps are represented by two
     * consecutive points sharing the same ``x``-coordinate. The first two and
     * last two points must have distinct ``x``-coordinates (jumps at the
     * endpoints are not supported). The first and last segments are used for
     * extrapolation beyond the endpoints.
     *
     * Parameters
     * ----------
     * points
     *     Ordered ``(x, f(x))`` points defining the function.
     *
     * Raises
     * ------
     * ValueError
     *     When fewer than two points are provided, when the first two or last
     *     two points share an ``x``-coordinate, when points are not
     *     non-decreasing in ``x``, or when the implied slope is not integral.
     */
    PiecewiseLinearFunction(std::vector<Point> points);

    // FIXME: consider removing this constructor once the points constructor is
    // the standard interface.
    /**
     * PiecewiseLinearFunction(
     *     breakpoints: list[np.int64],
     *     segments: list[tuple[np.int64, np.int64]],
     * )
     *
     * Creates a piecewise linear function from breakpoints and segments.
     *
     * Parameters
     * ----------
     * breakpoints
     *     Ordered, strictly increasing breakpoints.
     * segments
     *     Segment ``(intercept, slope)`` pairs. Must contain exactly one more
     *     segment than breakpoints.
     *
     * Raises
     * ------
     * ValueError
     *     When ``segments`` is empty, when there is not exactly one more
     *     segment than breakpoints, or when breakpoints are not strictly
     *     increasing.
     */
    PiecewiseLinearFunction(std::vector<Dom> breakpoints,
                            std::vector<Segment> segments);

    /**
     * PiecewiseLinearFunction()
     *
     * Creates the zero function represented by a single segment with zero slope
     * and zero intercept.
     */
    PiecewiseLinearFunction();

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
    : PiecewiseLinearFunction({}, {{0, 0}})
{
}

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Point> points)
{
    // FIXME: ask Niels whether a minimum of two points is preferred or a
    // single-point value should be supported.
    if (points.size() < 2)
        throw std::invalid_argument("Need at least two points.");

    if (points[0].first == points[1].first)
        throw std::invalid_argument(
            "First two points must have distinct x-coordinates.");

    if (points[points.size() - 2].first == points.back().first)
        throw std::invalid_argument(
            "Last two points must have distinct x-coordinates.");

    for (size_t idx = 0; idx + 1 < points.size(); ++idx)
    {
        if (points[idx + 1].first < points[idx].first)
            throw std::invalid_argument("Points must be non-decreasing in x.");

        if (points[idx].first < points[idx + 1].first)
        {
            auto const dy = points[idx + 1].second - points[idx].second;
            auto const dx = points[idx + 1].first - points[idx].first;

            if (dy % dx != 0)
                throw std::invalid_argument("Implied slope is not integral.");
        }
    }

    // Left extrapolation: slope of first segment (between first two points).
    auto const firstSlope = (points[1].second - points[0].second)
                            / (points[1].first - points[0].first);
    segments_.push_back(
        {points[0].second - firstSlope * points[0].first, firstSlope});

    // Add all distinct x-values as breakpoints, with the segment to the
    // right of each breakpoint.
    size_t i = 0;
    Dom prevSlope = firstSlope;

    while (i < points.size())
    {
        auto const x = points[i].first;

        // Find the last point at this x (handles jumps).
        size_t j = i;
        while (j + 1 < points.size() && points[j + 1].first == x)
            ++j;

        auto const yRight = points[j].second;
        Dom slopeRight;

        if (j + 1 < points.size())
        {
            auto const xNext = points[j + 1].first;
            auto const yNextLeft = points[j + 1].second;
            slopeRight = (yNextLeft - yRight) / (xNext - x);
            prevSlope = slopeRight;
        }
        else
        {
            // Right extrapolation: same slope as last segment.
            slopeRight = prevSlope;
        }

        breakpoints_.push_back(x);
        segments_.push_back({yRight - slopeRight * x, slopeRight});

        i = j + 1;
    }
}

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Dom> breakpoints, std::vector<Segment> segments)
    : breakpoints_(std::move(breakpoints)), segments_(std::move(segments))
{
    if (segments_.empty())
        throw std::invalid_argument("Need at least one segment.");

    if (breakpoints_.size() + 1 != segments_.size())
        throw std::invalid_argument("Need one more segment than breakpoints.");

    for (size_t idx = 0; idx + 1 < breakpoints_.size(); ++idx)
        if (breakpoints_[idx] >= breakpoints_[idx + 1])
            throw std::invalid_argument(
                "Breakpoints must be strictly increasing.");
}

template <typename Dom, typename Co>
Co PiecewiseLinearFunction<Dom, Co>::operator()(Dom x) const
{
    auto const idx = static_cast<size_t>(
        std::upper_bound(breakpoints_.begin(), breakpoints_.end(), x)
        - breakpoints_.begin());
    auto const [intercept, slope] = segments_[idx];
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
    for (auto const &[_, slope] : segments_)
    {
        if (slope < 0)
            return false;
    }

    for (size_t idx = 0; idx != breakpoints_.size(); ++idx)
    {
        auto const breakpoint = breakpoints_[idx];
        auto const [prevIntercept, prevSlope] = segments_[idx];
        auto const [nextIntercept, nextSlope] = segments_[idx + 1];

        auto const left = prevIntercept + prevSlope * breakpoint;
        auto const right = nextIntercept + nextSlope * breakpoint;

        if (right < left)
            return false;
    }

    return true;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
