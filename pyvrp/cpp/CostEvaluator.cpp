#include "CostEvaluator.h"

#include <limits>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

cost_type CostEvaluator::penalisedCost(Individual const &individual) const
{
    // Standard objective plus penalty terms for capacity- and time-related
    // infeasibilities.
    return static_cast<cost_type>(individual.distance())
           + individual.uncollectedPrizes()
           + loadPenaltyExcess(individual.excessLoad())
           + twPenalty(individual.timeWarp());
}

cost_type CostEvaluator::cost(Individual const &individual) const
{
    // Penalties are zero when individual is feasible, so we can fall back to
    // penalised cost in that case.
    return individual.isFeasible() ? penalisedCost(individual)
                                   : std::numeric_limits<value_type>::max();
}
