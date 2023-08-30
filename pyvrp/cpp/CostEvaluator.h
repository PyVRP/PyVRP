#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "Solution.h"

#include <concepts>
#include <limits>

namespace pyvrp
{
// The following methods must be implemented for a type to be evaluatable by
// the CostEvaluator.
template <typename T> concept CostEvaluatable = requires(T arg)
{
    // clang-format off
    { arg.distance() } -> std::same_as<Distance>;
    { arg.excessLoad() } -> std::same_as<Load>;
    { arg.fixedVehicleCost() }  -> std::same_as<Cost>;
    { arg.timeWarp() } -> std::same_as<Duration>;
    { arg.uncollectedPrizes() } -> std::same_as<Cost>;
    { arg.isFeasible() } -> std::same_as<bool>;
    // clang-format on
};

/**
 * CostEvaluator(capacity_penalty: int, tw_penalty: int)
 *
 * Creates a CostEvaluator instance.
 *
 * This class contains time warp and load penalties, and can compute penalties
 * for a given time warp and load.
 *
 * Parameters
 * ----------
 * capacity_penalty
 *    The penalty for each unit of excess load over the vehicle capacity.
 * tw_penalty
 *    The penalty for each unit of time warp.
 */
class CostEvaluator
{
    Cost capacityPenalty;
    Cost timeWarpPenalty;

public:
    CostEvaluator(Cost capacityPenalty, Cost timeWarpPenalty);

    /**
     * Computes the total excess capacity penalty for the given load.
     */
    [[nodiscard]] inline Cost loadPenalty(Load load, Load capacity) const;

    /**
     * Computes the excess capacity penalty for the given excess load, that is,
     * the part of the load that exceeds the capacity.
     */
    [[nodiscard]] inline Cost loadPenaltyExcess(Load excessLoad) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline Cost twPenalty(Duration timeWarp) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given solution.
     */
    // The docstring above is written for Python, where we only expose this
    // method for Solution.
    template <CostEvaluatable T>
    [[nodiscard]] Cost penalisedCost(T const &arg) const;

    /**
     * Hand-waving some details, each solution consists of a set of routes
     * :math:`\mathcal{R}`. Each route :math:`R \in \mathcal{R}` is a sequence
     * of edges, starting and ending at the depot. A route :math:`R` has an
     * assigned vehicle type :math:`t_R`, which has a fixed vehicle cost
     * :math:`f_{t_R}`. Let :math:`V_R = \{i : (i, j) \in R \}` be the set of
     * locations visited by route :math:`R`. The objective value is then given
     * by
     *
     * .. math::
     *
     *    \sum_{R \in \mathcal{R}} \left[ f_{t_R}
     *          + \sum_{(i, j) \in R} d_{ij} \right]
     *    + \sum_{i \in V} p_i - \sum_{R \in \mathcal{R}} \sum_{i \in V_R} p_i,
     *
     * where the first part lists the vehicle and distance costs, and the
     * second part the uncollected prizes of unvisited clients.
     *
     * .. note::
     *
     *    The above cost computation only holds for feasible solutions. If the
     *    solution argument is *infeasible*, we return a very large number.
     *    If that is not what you want, consider calling :meth:`penalised_cost`
     *    instead.
     */
    // The docstring above is written for Python, where we only expose this
    // method for Solution.
    template <CostEvaluatable T> [[nodiscard]] Cost cost(T const &arg) const;
};

Cost CostEvaluator::loadPenaltyExcess(Load excessLoad) const
{
    return static_cast<Cost>(excessLoad) * capacityPenalty;
}

Cost CostEvaluator::loadPenalty(Load load, Load capacity) const
{
    // Branchless for performance: when load > capacity we return the excess
    // load penalty; else zero. Note that when load - capacity wraps
    // around, we return zero because load > capacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    Cost penalty = loadPenaltyExcess(load - capacity);
    return Cost(load > capacity) * penalty;
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp) * timeWarpPenalty;
#endif
}

template <CostEvaluatable T>
Cost CostEvaluator::penalisedCost(T const &arg) const
{
    // Standard objective plus penalty terms for capacity- and time-related
    // infeasibilities.
    // clang-format off
    return static_cast<Cost>(arg.distance())
           + arg.fixedVehicleCost()
           + arg.uncollectedPrizes()
           + loadPenaltyExcess(arg.excessLoad())
           + twPenalty(arg.timeWarp());
    // clang-format on
}

template <CostEvaluatable T> Cost CostEvaluator::cost(T const &arg) const
{
    // Penalties are zero when the solution is feasible, so we can fall back to
    // penalised cost in that case.
    return arg.isFeasible() ? penalisedCost(arg)
                            : std::numeric_limits<Cost>::max();
}
}  // namespace pyvrp

#endif  // PYVRP_COSTEVALUATOR_H
