#include "CostEvaluator.h"

#include <numeric>

CostEvaluator::CostEvaluator(unsigned int capacityPenalty,
                             unsigned int timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}

unsigned int CostEvaluator::operator()(Individual const &individual) const
{
    auto const loadPen = loadPenaltyExcess(individual.excessLoad());
    auto const twPen = twPenalty(individual.timeWarp());

    return individual.distance() + loadPen + twPen;
}
