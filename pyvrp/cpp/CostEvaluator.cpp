#include "CostEvaluator.h"

using pyvrp::CostEvaluator;

CostEvaluator::CostEvaluator(std::vector<double> loadPenalties,
                             double twPenalty,
                             double distPenalty)
    : loadPenalties_(std::move(loadPenalties)),
      twPenalty_(twPenalty),
      distPenalty_(distPenalty)
{
}
