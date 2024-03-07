#include "CostEvaluator.h"

using pyvrp::CostEvaluator;

CostEvaluator::CostEvaluator(Cost loadPenalty, Cost twPenalty, Cost distPenalty)
    : loadPenalty_(loadPenalty),
      twPenalty_(twPenalty),
      distPenalty_(distPenalty)
{
}
