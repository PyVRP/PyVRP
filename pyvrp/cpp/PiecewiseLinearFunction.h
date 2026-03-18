#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pyvrp
{
// clang-format off
/**
 * PiecewiseLinearFunction(points: list[tuple[np.int64, np.int64]])
 *
 * Creates a piecewise linear function :math:`f` from :math:`(x, f(x))`
 * points that are non-decreasing in :math:`x`.
 *
 * .. note::
 *    The interface follows
 *    `Gurobi <https://docs.gurobi.com/projects/optimizer/en/current/concepts/modeling/objectives.html#linear-objectives>`_.
 *    Jumps are supported, single points are not.
 *
 * The points are converted into an internal representation with breakpoints
 * and segments. In particular, :math:`f` is represented via :math:`n - 1`
 * strictly increasing breakpoints :math:`(b_i)_{i = 1, \ldots, n - 1}` and
 * :math:`n` linear segments :math:`(f_i)_{i = 1, \ldots, n}`. Given :math:`x`,
 * define
 *
 * .. math::
 *    f(x) =
 *    \begin{cases}
 *       f_1(x) & \text{ if } x < b_1, \\
 *       f_i(x) & \text{ if } b_{i - 1} \le x \le b_i
 *                \text{ for } i = 2, \ldots, n - 1, \\
 *       f_n(x) & \text{ otherwise.}
 *    \end{cases}
 *
 * Parameters
 * ----------
 * points
 *     Ordered :math:`(x, f(x))` points defining the function :math:`f`.
 *
 * Raises
 * ------
 * ValueError
 *     When fewer than two points are provided, when the points are decreasing
 *     in :math:`x`, or when the implied slope between points is not integral.
 */
// clang-format on
template <typename Dom, typename Co> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Dom, Dom>;
    using Point = std::pair<Dom, Co>;

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;

public:
    // Construct from points.
    PiecewiseLinearFunction(std::vector<Point> points);

    // Directly construct from breakpoints and segments.
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
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Point> points)
{
    if (points.size() < 2)
        throw std::invalid_argument("Need at least two points.");

    if (points[0].first == points[1].first)
        throw std::invalid_argument(
            "First two points must have distinct x-coordinates.");

    if (points[points.size() - 2].first == points.back().first)
        throw std::invalid_argument(
            "Last two points must have distinct x-coordinates.");

    for (size_t idx = 0; idx + 1 != points.size(); ++idx)
    {
        auto const curr = points[idx];
        auto const next = points[idx + 1];

        if (curr.first == next.first)  // nothing to do; we have a jump at this
            continue;                  // point, but not a new segment.

        auto const dy = next.second - curr.second;
        auto const dx = next.first - curr.first;

        if (dx < 0)
            throw std::invalid_argument("Points must be non-decreasing in x.");

        if (dy % dx != 0)
            throw std::invalid_argument("Slope is not integral.");

        if (idx != 0)  // breakpoints separate segments
            breakpoints_.push_back(curr.first);

        auto const slope = dy / dx;
        auto const intercept = curr.second - slope * curr.first;
        segments_.emplace_back(intercept, slope);
    }

    assert(!segments_.empty());
    assert(breakpoints_.size() + 1 == segments_.size());
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
    auto const idx = std::distance(
        breakpoints_.begin(),
        std::upper_bound(breakpoints_.begin(), breakpoints_.end(), x));

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
        if (slope < 0)
            return false;

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
