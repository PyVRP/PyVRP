#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "ProblemData.h"
#include "Solution.h"
#include "TimeWindowSegment.h"

using VehicleType = ProblemData::VehicleType;

namespace pyvrp
{
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
     * Computes the objective (penalised cost) for a route given set of
     * properties.
     */
    [[nodiscard]] inline Cost
    penalisedRouteCost(Distance const distance,
                       Load const load,
                       TimeWindowSegment const &tws,
                       ProblemData::VehicleType const &vehicleType) const;

    /**
     * Computes the objective (penalised cost) for a route given set of
     * properties.
     */
    [[nodiscard]] inline Cost
    penalisedRouteCost(Distance const distance,
                       Load const load,
                       Duration const duration,
                       Duration const timeWarp,
                       ProblemData::VehicleType const &vehicleType) const;

    /**
     * Computes the objective (penalised cost) for a given solution.
     */
    [[nodiscard]] Cost penalisedCost(Solution const &solution) const;

    /**
     * Evaluates and returns the cost/objective of the given solution.
     * Hand-waving some details, let :math:`x_{ij} \in \{ 0, 1 \}` indicate
     * if edge :math:`(i, j)` is used in the solution encoded by the given
     * solution, and :math:`y_i \in \{ 0, 1 \}` indicate if client
     * :math:`i` is visited. The objective is then given by
     *
     * .. math::

     *    \sum_{(i, j)} d_{ij} x_{ij} + \sum_{i} p_i (1 - y_i),
     *
     * where the first part lists the distance costs, and the second part the
     * prizes of the unvisited clients.
     */
    [[nodiscard]] Cost cost(Solution const &solution) const;
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
}  // namespace pyvrp

Cost CostEvaluator::penalisedRouteCost(
    Distance const distance,
    Load const load,
    TimeWindowSegment const &tws,
    ProblemData::VehicleType const &vehicleType) const
{
    return penalisedRouteCost(
        distance, load, tws.totalDuration(), tws.totalTimeWarp(), vehicleType);
}

Cost CostEvaluator::penalisedRouteCost(
    Distance const distance,
    Load const load,
    Duration const duration,
    Duration const timeWarp,
    ProblemData::VehicleType const &vehicleType) const
{
    auto const loadPen = loadPenalty(load, vehicleType.capacity);
    auto const twPen = twPenalty(timeWarp);
    auto const distanceCost
        = vehicleType.costPerDistance * static_cast<Cost>(distance);
    auto const durationCost
        = vehicleType.costPerDuration * static_cast<Cost>(duration);

    return distanceCost + durationCost + loadPen + twPen;
}

#endif  // PYVRP_COSTEVALUATOR_H
