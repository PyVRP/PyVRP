#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <cassert>
#include <limits>
#include <stdexcept>
#include <tuple>
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
    using Point = std::tuple<Dom, Dom, Dom>;

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;

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

    for (size_t idx = 0; idx != points.size() - 1; ++idx)
        if (std::get<0>(points[idx]) >= std::get<0>(points[idx + 1]))
            throw std::invalid_argument(
                "Points must be strictly increasing in breakpoint.");

    std::vector<Dom> values(points.size());
    values[0] = std::get<2>(points[0]);  // f(x_0) equals the first jump value.

    for (size_t idx = 1; idx != points.size(); ++idx)
    {
        auto const dx = std::get<0>(points[idx]) - std::get<0>(points[idx - 1]);
        auto const delta = std::get<1>(points[idx - 1]) * dx;
        auto const leftValue = values[idx - 1] + delta;
        values[idx] = leftValue + std::get<2>(points[idx]);
    }

    breakpoints_.reserve(points.size() + 1);
    for (auto const &point : points)
        breakpoints_.push_back(std::get<0>(point));

    // Duplicate the final breakpoint so jumps at the last point are modeled.
    breakpoints_.push_back(std::get<0>(points.back()));

    auto const segmentFromAnchor = [&](size_t idx)
    {
        auto const breakpoint = std::get<0>(points[idx]);
        auto const slope = std::get<1>(points[idx]);
        auto const product = slope * breakpoint;
        auto const intercept = values[idx] - product;
        return Segment{intercept, slope};
    };

    segments_.reserve(points.size() + 1);
    segments_.push_back(segmentFromAnchor(0));  // left-side extrapolation

    for (size_t idx = 0; idx != points.size() - 1; ++idx)
        segments_.push_back(segmentFromAnchor(idx));

    segments_.push_back(segmentFromAnchor(points.size() - 1));
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
        auto const breakpoint = static_cast<Co>(breakpoints_[idx - 1]);
        auto const [prevIntercept, prevSlope] = segments_[idx - 1];
        auto const [nextIntercept, nextSlope] = segments_[idx];

        auto const left = static_cast<Co>(prevIntercept)
                          + static_cast<Co>(prevSlope) * breakpoint;
        auto const right = static_cast<Co>(nextIntercept)
                           + static_cast<Co>(nextSlope) * breakpoint;

        if (right < left)
            return false;
    }

    return true;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
