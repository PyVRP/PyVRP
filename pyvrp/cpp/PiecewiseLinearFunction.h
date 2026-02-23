#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <cassert>
#include <fstream>
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
 * Creates a piecewise linear function :math:`f`. TODO
 *
 * Segment :math:`f_i`, breakpoints :math:`b_i`. TODO
 *
 * Parameters
 * ----------
 * breakpoints
 *     TODO
 * segments
 *     TODO
 */
template <typename Dom, typename Co> class PiecewiseLinearFunction
{
public:
    using Segment = std::pair<Dom, Dom>;

private:
    std::vector<Dom> breakpoints_;
    std::vector<Segment> segments_;

public:
    PiecewiseLinearFunction(std::vector<Dom> breakpoints,
                            std::vector<Segment> segments);

    /**
     * Evaluates :math:`f(x)`.
     */
    [[nodiscard]] inline Co operator()(Dom x) const;

    /**
     * Breakpoints :math:`b_i`.
     */
    [[nodiscard]] std::vector<Dom> const &breakpoints() const;

    /**
     * Intercept and slope pairs for each segment :math:`f_i`.
     */
    [[nodiscard]] std::vector<Segment> const &segments() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

template <typename Dom, typename Co>
PiecewiseLinearFunction<Dom, Co>::PiecewiseLinearFunction(
    std::vector<Dom> breakpoints, std::vector<Segment> segments)
    : breakpoints_(std::move(breakpoints)), segments_(std::move(segments))
{
    if (breakpoints_.size() < 2)
        throw std::invalid_argument("Need at least two breakpoints.");

    if (breakpoints_.size() - 1 != segments.size())
        throw std::invalid_argument(
            "There must be one more breakpoint than the number of segments.");

    for (size_t idx = 0; idx != breakpoints_.size() - 1; ++idx)
        if (breakpoints_[idx] >= breakpoints_[idx + 1])
            throw std::invalid_argument(
                "Breakpoints must be strictly increasing.");

    auto const min = std::numeric_limits<Dom>::min();
    auto const max = std::numeric_limits<Dom>::max();
    if (breakpoints_.front() != min || breakpoints_.back() != max)
    {
        std::ostringstream msg;
        msg << "Breakpoints must cover full domain range [min, max]:" << " ["
            << breakpoints_[0] << ", " << breakpoints_.back() << ']'
            << " does not cover [ " << min << ", " << max << "].";
        throw std::invalid_argument(msg.str());
    }
}

template <typename Dom, typename Co>
Co PiecewiseLinearFunction<Dom, Co>::operator()(Dom x) const
{
    for (size_t idx = 0; idx != breakpoints_.size() - 1; ++idx)
    {
        auto const leftBreakpoint = breakpoints_[idx];
        auto const rightBreakpoint = breakpoints_[idx + 1];

        if (leftBreakpoint <= x && x < rightBreakpoint)
        {
            auto const [intercept, slope] = segments_[idx];
            return static_cast<Co>(intercept + slope * x);
        }
    }

    assert(x == std::numeric_limits<Dom>::max());  // rare edge case
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
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
