#include "CostEvaluator.h"

#include <limits>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

Cost CostEvaluator::penalisedCost(Individual const &individual) const
{
    // Standard objective plus penalty terms for capacity- and time-related
    // infeasibilities.
    return static_cast<Cost>(individual.distance())
           + individual.uncollectedPrizes()
           + loadPenaltyExcess(individual.excessLoad())
           + twPenalty(individual.timeWarp());
}

Cost CostEvaluator::cost(Individual const &individual) const
{
    // Penalties are zero when individual is feasible, so we can fall back to
    // penalised cost in that case.
    return individual.isFeasible() ? penalisedCost(individual)
                                   : std::numeric_limits<Value>::max();
}
