#ifndef PYVRP_DURATIONCOSTFUNCTION_H
#define PYVRP_DURATIONCOSTFUNCTION_H

#include "Measure.h"
#include "PiecewiseLinearFunction.h"

#include <vector>

// NOTE: #925/1044-FormPup41
// We can later extend this class with other duration cost functions, e.g.
// polynomial methods.
// For now, it only wraps a piecewise linear function and validates
// duration-specific invariants.

namespace pyvrp
{
/**
 * DurationCostFunction(
 *     breakpoints: list[int] = [0],
 *     slopes: list[int] = [0],
 * )
 *
 * Creates a duration cost function that maps durations to costs.
 *
 * This class wraps a :class:`~pyvrp._pyvrp.PiecewiseLinearFunction` and
 * validates duration-specific invariants. In particular:
 *
 * - breakpoints and slopes must be non-negative;
 * - the first breakpoint must be duration ``0``;
 * - slopes must be non-decreasing (convex cumulative cost), to avoid
 *   non-convex regions where extending a route could reduce total duration
 *   cost;
 * - intercept is fixed to ``0`` for duration costs.
 *
 * ``operator()`` returns the cumulative total cost at the given duration,
 * not a marginal rate. For example, with ``breakpoints = [0, 30]`` and
 * ``slopes = [1, 2]``:
 *
 * - cost at duration 15 is ``15``;
 * - cost at duration 45 is ``30 * 1 + 15 * 2 = 60``.
 *
 * Parameters
 * ----------
 * breakpoints
 *     Strictly increasing segment start durations. The first breakpoint must
 *     be ``0``.
 * slopes
 *     Segment slopes. Must have the same length as ``breakpoints``, and must
 *     be non-decreasing to keep the duration cost function convex.
 */
class DurationCostFunction
{
    PiecewiseLinearFunction pwl_;

public:
    DurationCostFunction(std::vector<Duration> breakpoints = {0},
                         std::vector<Cost> slopes = {0});

    /**
     * Builds a duration cost function from the legacy linear and overtime
     * parameters, preserving legacy semantics exactly:
     *
     * ``cost(d) = unit_duration_cost * d``
     * ``          + unit_overtime_cost * max(0, d - shift_duration)``.
     *
     * This yields:
     *
     * - slope ``unit_duration_cost`` on ``[0, shift_duration)``;
     * - slope ``unit_duration_cost + unit_overtime_cost`` on
     *   ``[shift_duration, +inf)``.
     *
     * Special cases:
     *
     * - if ``shift_duration == 0``, overtime is active from duration 0 and the
     *   single slope is ``unit_duration_cost + unit_overtime_cost``;
     * - if ``shift_duration`` equals the maximum representable duration
     *   (``int64`` max), overtime never activates for representable durations,
     *   so no second segment is added.
     *
     * Example: ``shift_duration = 10``, ``unit_duration_cost = 2``,
     * ``unit_overtime_cost = 5``:
     *
     * - ``cost(6) = 12``
     * - ``cost(10) = 20``
     * - ``cost(13) = 2 * 13 + 5 * (13 - 10) = 41``.
     *
     * Raises
     * ------
     * OverflowError
     *     If ``unit_duration_cost + unit_overtime_cost`` cannot be represented
     *     as an ``int64`` cost.
     */
    [[nodiscard]] static DurationCostFunction fromLinear(Duration shiftDuration,
                                                         Cost unitDurationCost,
                                                         Cost unitOvertimeCost);

    /**
     * Builds a duration cost function from a generic piecewise linear
     * function.
     */
    explicit DurationCostFunction(PiecewiseLinearFunction pwl);

    /**
     * Evaluates the cumulative total duration cost at ``duration``.
     */
    [[nodiscard]] Cost operator()(Duration duration) const;

    /**
     * Segment breakpoints in duration units.
     */
    [[nodiscard]] std::vector<Duration> breakpoints() const;

    /**
     * Segment slopes in cost-per-duration units.
     */
    [[nodiscard]] std::vector<Cost> slopes() const;

    /**
     * Function values at each breakpoint.
     */
    [[nodiscard]] std::vector<Cost> values() const;

    /**
     * Returns the wrapped piecewise linear function.
     */
    [[nodiscard]] PiecewiseLinearFunction const &piecewiseLinear() const;

    /**
     * Tests whether this function is identically zero.
     */
    [[nodiscard]] bool isZero() const;

    /**
     * Returns a linear proxy slope (cost per duration unit) for edge-based
     * heuristics. For the current piecewise-linear implementation this is the
     * slope of the first segment.
     */
    [[nodiscard]] Cost edgeCostSlope() const;

    bool operator==(DurationCostFunction const &other) const = default;
};

inline PiecewiseLinearFunction const &
DurationCostFunction::piecewiseLinear() const
{
    return pwl_;
}

inline bool DurationCostFunction::isZero() const { return pwl_.isZero(); }
}  // namespace pyvrp

#endif  // PYVRP_DURATIONCOSTFUNCTION_H
