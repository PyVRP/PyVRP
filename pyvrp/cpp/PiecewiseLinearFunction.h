#ifndef PYVRP_PIECEWISELINEARFUNCTION_H
#define PYVRP_PIECEWISELINEARFUNCTION_H

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace pyvrp
{
/**
 * PiecewiseLinearFunction(
 *     breakpoints: list[int] = [0],
 *     slopes: list[int] = [0],
 *     intercept: int = 0,
 * )
 *
 * Creates a piecewise linear function over integer inputs.
 *
 * The given ``breakpoints`` define the start of each segment. Segment ``i``
 * starts at ``breakpoints[i]`` and uses slope ``slopes[i]``. Segments are
 * left-closed and right-open, except for the final segment, which is
 * unbounded to the right.
 *
 * Let ``b_i`` denote breakpoints, ``s_i`` segment slopes, and ``v_i`` the
 * function value at ``b_i``. This class stores and evaluates the cumulative
 * function value:
 *
 * - ``v_0 = intercept``
 * - ``v_{i + 1} = v_i + s_i * (b_{i + 1} - b_i)``
 * - for ``x`` in ``[b_i, b_{i + 1})``:
 *   ``f(x) = v_i + s_i * (x - b_i)``
 *
 * Example (cumulative total cost semantics):
 *
 * - ``breakpoints = [0, 30]``
 * - ``slopes = [1, 2]``
 * - ``intercept = 0``
 *
 * Then ``f(15) = 15 * 1`` and ``f(45) = 30 * 1 + 15 * 2 = 60``.
 *
 * Parameters
 * ----------
 * breakpoints
 *     Strictly increasing list of segment start points.
 * slopes
 *     Segment slopes. Must have the same length as ``breakpoints``.
 * intercept
 *     Function value at ``breakpoints[0]``.
 */
class PiecewiseLinearFunction
{
public:
    using Scalar = int64_t;

private:
    std::vector<Scalar> breakpoints_;
    std::vector<Scalar> slopes_;
    std::vector<Scalar> values_;  // function values at breakpoints
    Scalar intercept_ = 0;

public:
    PiecewiseLinearFunction(
        std::vector<Scalar> breakpoints = {0},
        std::vector<Scalar> slopes = {0},
        Scalar intercept = 0
    );

    /**
     * Evaluates the cumulative function value at ``x``.
     *
     * Raises
     * ------
     * ValueError
     *     If ``x`` is smaller than the first breakpoint, because the function
     *     domain starts at ``breakpoints[0]``.
     */
    [[nodiscard]] Scalar operator()(Scalar x) const;

    /**
     * Segment breakpoints of this function.
     */
    [[nodiscard]] std::vector<Scalar> const &breakpoints() const;

    /**
     * Segment slopes of this function.
     */
    [[nodiscard]] std::vector<Scalar> const &slopes() const;

    /**
     * Function values at each breakpoint.
     */
    [[nodiscard]] std::vector<Scalar> const &values() const;

    /**
     * Function value at the first breakpoint.
     */
    [[nodiscard]] Scalar intercept() const;

    /**
     * Tests whether this function is the zero function.
     */
    [[nodiscard]] bool isZero() const;

    bool operator==(PiecewiseLinearFunction const &other) const = default;
};

inline std::vector<PiecewiseLinearFunction::Scalar> const &
PiecewiseLinearFunction::breakpoints() const
{
    return breakpoints_;
}

inline std::vector<PiecewiseLinearFunction::Scalar> const &
PiecewiseLinearFunction::slopes() const
{
    return slopes_;
}

inline std::vector<PiecewiseLinearFunction::Scalar> const &
PiecewiseLinearFunction::values() const
{
    return values_;
}

inline PiecewiseLinearFunction::Scalar PiecewiseLinearFunction::intercept() const
{
    return intercept_;
}
}  // namespace pyvrp

#endif  // PYVRP_PIECEWISELINEARFUNCTION_H
