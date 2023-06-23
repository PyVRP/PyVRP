#include "CostEvaluator.h"

#include <limits>

CostEvaluator::CostEvaluator(Cost weightCapacityPenalty, Cost volumeCapacityPenalty, Cost timeWarpPenalty)
    : weightCapacityPenalty(weightCapacityPenalty), volumeCapacityPenalty(volumeCapacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

Cost CostEvaluator::penalisedCost(Solution const &solution) const
{
    // Standard objective plus penalty terms for weight-, volume-, and time-related
    // infeasibilities.
    return static_cast<Cost>(solution.distance()) + solution.uncollectedPrizes()
           + weightPenaltyExcess(solution.excessWeight())
           + volumePenaltyExcess(solution.excessVolume())
           + twPenalty(solution.timeWarp());
}

Cost CostEvaluator::cost(Solution const &solution) const
{
    // Penalties are zero when the solution is feasible, so we can fall back to
    // penalised cost in that case.
    return solution.isFeasible() ? penalisedCost(solution)
                                 : std::numeric_limits<Cost>::max();
}
