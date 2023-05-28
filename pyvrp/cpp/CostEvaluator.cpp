#include "CostEvaluator.h"

#include <limits>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

unsigned int CostEvaluator::penalisedCost(Individual const &individual) const
{
    // Standard objective plus penalty terms for capacity- and time-related
    // infeasibilities.
    return individual.distance() + individual.uncollectedPrizes()
           + loadPenaltyExcess(individual.excessLoad())
           + twPenalty(individual.timeWarp());
}

unsigned int CostEvaluator::cost(Individual const &individual) const
{
    return individual.isFeasible()
               ? individual.distance() + individual.uncollectedPrizes()
               : std::numeric_limits<unsigned int>::max();
}
