#include "CostEvaluator.h"
#include <limits>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

unsigned int CostEvaluator::penalizedCost(Individual const &individual) const
{
    auto const loadPen = loadPenaltyExcess(individual.excessLoad());
    auto const twPen = twPenalty(individual.timeWarp());

    return individual.distance() + loadPen + twPen;
}

unsigned int CostEvaluator::cost(Individual const &individual) const
{
    return individual.isFeasible() ? individual.distance()
                                   : std::numeric_limits<unsigned int>::max();
}
