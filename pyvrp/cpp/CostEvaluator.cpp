#include "CostEvaluator.h"
#include <limits>

CostEvaluator::CostEvaluator(cost_type capacityPenalty,
                             cost_type timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

cost_type CostEvaluator::penalisedCost(Individual const &individual) const
{
    auto const loadPen = loadPenaltyExcess(individual.excessLoad());
    auto const twPen = twPenalty(individual.timeWarp());

    return individual.distance() + loadPen + twPen;
}

cost_type CostEvaluator::cost(Individual const &individual) const
{
    return individual.isFeasible() ? individual.distance()
                                   : std::numeric_limits<cost_type>::max();
}
