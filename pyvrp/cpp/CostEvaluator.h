#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "ProblemData.h"
#include "Solution.h"
#include "TimeWindowSegment.h"

using VehicleType = ProblemData::VehicleType;

/**
 * Cost evaluator class that computes penalty values for timewarp and load.
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
     * Computes the objective for a given solution. Returns the largest
     * representable cost value if the solution is infeasible.
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
