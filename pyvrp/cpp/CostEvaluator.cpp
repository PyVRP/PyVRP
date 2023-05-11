#include "CostEvaluator.h"
#include <limits>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

int CostEvaluator::penalisedCost(Individual const &individual) const
{
    auto const loadPen = loadPenaltyExcess(individual.excessLoad());
    auto const twPen = twPenalty(individual.timeWarp());

    return individual.distance() + loadPen + twPen + individual.uncollected();
}

int CostEvaluator::cost(Individual const &individual) const
{
    return individual.isFeasible()
               ? individual.distance() + individual.uncollected()
               : std::numeric_limits<int>::max();
}
