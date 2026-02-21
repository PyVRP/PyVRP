#include "DurationCostFunction.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <vector>

using pyvrp::Cost;
using pyvrp::Duration;
using pyvrp::DurationCostFunction;
using pyvrp::PiecewiseLinearFunction;

namespace
{
template <typename Measure>
std::vector<PiecewiseLinearFunction::Scalar>
toRaw(std::vector<Measure> const &vec)
{
    std::vector<PiecewiseLinearFunction::Scalar> raw;
    raw.reserve(vec.size());

    for (auto const value : vec)
        raw.push_back(value.get());

    return raw;
}

// Internal helper used only by fromLinear() below, which is currently a
// compatibility path not reachable from the public Python API.
// GCOVR_EXCL_START
[[nodiscard]] Cost checkedAddCost(Cost lhs, Cost rhs)
{
    using Scalar = PiecewiseLinearFunction::Scalar;
    auto constexpr MAX = std::numeric_limits<Scalar>::max();
    auto constexpr MIN = std::numeric_limits<Scalar>::min();

    auto const lhsRaw = lhs.get();
    auto const rhsRaw = rhs.get();

    if ((rhsRaw > 0 && lhsRaw > MAX - rhsRaw)
        || (rhsRaw < 0 && lhsRaw < MIN - rhsRaw))
    {
        auto const *msg = "unit_duration_cost + unit_overtime_cost overflows.";
        throw std::overflow_error(msg);
    }

    return lhsRaw + rhsRaw;
}
// GCOVR_EXCL_STOP

void validateDurationPwl(PiecewiseLinearFunction const &pwl)
{
    auto const &breakpoints = pwl.breakpoints();
    auto const &slopes = pwl.slopes();

    // FIXME:: #925/1044-FormPup41:
    // It is possible to allow a non-zero first breakpoint, but then the
    // duration cost function must also define behavior on [0,
    // first_breakpoint), for example via extrapolation or clamping. For now, we
    // require that duration costs are directly defined from duration 0 onward.
    if (breakpoints.front() != 0)
        throw std::invalid_argument("breakpoints must start at 0.");

    // Unreachable in current implementation: PiecewiseLinearFunction enforces
    // sorted breakpoints and we already require breakpoints.front() == 0.
    // GCOVR_EXCL_START
    if (std::any_of(breakpoints.begin(),
                    breakpoints.end(),
                    [](auto const bp) { return bp < 0; }))
        throw std::invalid_argument("breakpoints must be >= 0.");
    // GCOVR_EXCL_STOP

    if (std::any_of(slopes.begin(),
                    slopes.end(),
                    [](auto const slope) { return slope < 0; }))
        throw std::invalid_argument("slopes must be >= 0.");

    // Non-decreasing slopes ensure convexity: each additional duration unit is
    // at least as expensive as the previous one.
    if (!std::is_sorted(slopes.begin(), slopes.end()))
        throw std::invalid_argument("slopes must be non-decreasing.");

    if (pwl.intercept() != 0)
        throw std::invalid_argument(
            "duration costs must have intercept 0 (no extra fixed cost).");
}
}  // namespace

DurationCostFunction::DurationCostFunction(std::vector<Duration> breakpoints,
                                           std::vector<Cost> slopes)
    : pwl_(toRaw(breakpoints), toRaw(slopes), 0)
{
    validateDurationPwl(pwl_);
}

DurationCostFunction::DurationCostFunction(PiecewiseLinearFunction pwl)
    : pwl_(std::move(pwl))
{
    validateDurationPwl(pwl_);
}

// Internal compatibility helper for legacy scalar duration/overtime costs.
// Intentionally not exposed in the public Python API to avoid confusion and
// encourage users to migrate to the more flexible duration cost function.
// GCOVR_EXCL_START
DurationCostFunction DurationCostFunction::fromLinear(Duration shiftDuration,
                                                      Cost unitDurationCost,
                                                      Cost unitOvertimeCost)
{
    if (shiftDuration < 0)
        throw std::invalid_argument("shift_duration must be >= 0.");

    if (unitDurationCost < 0)
        throw std::invalid_argument("unit_duration_cost must be >= 0.");

    if (unitOvertimeCost < 0)
        throw std::invalid_argument("unit_overtime_cost must be >= 0.");

    std::vector<Duration> breakpoints = {0};
    std::vector<Cost> slopes = {unitDurationCost};
    
    auto const hasFiniteOvertimeThreshold
        = shiftDuration > 0
          && shiftDuration < std::numeric_limits<Duration>::max();

    if (unitOvertimeCost != 0 && hasFiniteOvertimeThreshold)
    {
        breakpoints.push_back(shiftDuration);
        slopes.push_back(checkedAddCost(unitDurationCost, unitOvertimeCost));
    }
    else if (shiftDuration == 0 && unitOvertimeCost != 0)
    {
        // If shift_duration is zero, every duration unit is overtime. The
        // legacy expression then simplifies to:
        // cost(d) = (unitDurationCost + unitOvertimeCost) * d.
        slopes[0] = checkedAddCost(slopes[0], unitOvertimeCost);
    }

    return {std::move(breakpoints), std::move(slopes)};
}
// GCOVR_EXCL_STOP

Cost DurationCostFunction::operator()(Duration duration) const
{
    if (duration < 0)
        throw std::invalid_argument("duration must be >= 0.");

    return pwl_(duration.get());
}  // GCOVR_EXCL_LINE

std::vector<Duration> DurationCostFunction::breakpoints() const
{
    std::vector<Duration> breakpoints;
    breakpoints.reserve(pwl_.breakpoints().size());

    for (auto const bp : pwl_.breakpoints())
        breakpoints.push_back(bp);

    return breakpoints;
}

std::vector<Cost> DurationCostFunction::slopes() const
{
    std::vector<Cost> slopes;
    slopes.reserve(pwl_.slopes().size());

    for (auto const slope : pwl_.slopes())
        slopes.push_back(slope);

    return slopes;
}

std::vector<Cost> DurationCostFunction::values() const
{
    std::vector<Cost> values;
    values.reserve(pwl_.values().size());

    for (auto const value : pwl_.values())
        values.push_back(value);

    return values;
}

Cost DurationCostFunction::edgeCostSlope() const
{
    auto const &slopes = pwl_.slopes();
    assert(!slopes.empty());  // GCOVR_EXCL_LINE
    return slopes.front();
}
