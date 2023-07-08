#include "CostEvaluator.h"

#include <limits>

using pyvrp::Cost;
using pyvrp::CostEvaluator;
using pyvrp::Solution;

CostEvaluator::CostEvaluator(Cost capacityPenalty, Cost timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

Cost CostEvaluator::penalisedCost(Solution const &solution) const
{
    // Standard objective plus penalty terms for capacity- and time-related
    // infeasibilities.
    return static_cast<Cost>(solution.distance()) + solution.uncollectedPrizes()
           + loadPenaltyExcess(solution.excessLoad())
           + twPenalty(solution.timeWarp());
}

Cost CostEvaluator::cost(Solution const &solution) const
{
    // Penalties are zero when the solution is feasible, so we can fall back to
    // penalised cost in that case.
    return solution.isFeasible() ? penalisedCost(solution)
                                 : std::numeric_limits<Cost>::max();
}
