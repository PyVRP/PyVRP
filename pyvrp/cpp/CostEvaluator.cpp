#include "CostEvaluator.h"

using pyvrp::CostEvaluator;

CostEvaluator::CostEvaluator(double loadPenalty,
                             double twPenalty,
                             double distPenalty)
    : loadPenalty_({loadPenalty}),
      twPenalty_(twPenalty),
      distPenalty_(distPenalty)
{
}
