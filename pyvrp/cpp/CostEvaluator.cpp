#include "CostEvaluator.h"

using pyvrp::Cost;
using pyvrp::CostEvaluator;

CostEvaluator::CostEvaluator(Cost capacityPenalty, Cost timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}
