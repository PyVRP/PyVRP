#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pyvrp
{
/**
 * PiecewiseLinearFunction(
 *     breakpoints: list[np.int64],
 *     segments: list[tuple[np.int64, np.int64]],
 * )
 *
 * Creates a piecewise linear function :math:`f`. The piecewise linear function
 * is defined via :math:`n` strictly increasing breakpoints and linear segments
 * :math:`(b_i, f_i)_{i = 1, \ldots, n}`. Given :math:`x`, define
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
 * breakpoints
 *     Ordered, strictly increasing breakpoints.
 * segments
 *     The (intercept, slope) pairs that define each linear segment.
 */
template <typename Dom, typename Co> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Dom, Dom>;

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;
    bool isZero_ = false;

public:
    PiecewiseLinearFunction();

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
     * Returns whether this function is identically zero.
     */
    [[nodiscard]] bool isZero() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction()
    : PiecewiseLinearFunction({std::numeric_limits<Dom>::min()}, {{0, 0}})
{
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
        if (breakpoints_[idx] >= breakpoints_[idx + 1])
            throw std::invalid_argument(
                "Breakpoints must be strictly increasing.");

    isZero_ = std::all_of(segments_.begin(),
                          segments_.end(),
                          [](Segment const &segment)
                          { return segment.first == 0 && segment.second == 0; });
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
bool PiecewiseLinearFunction<Dom, Co>::isZero() const
{
    return isZero_;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
