#include "CostEvaluator.h"

using pyvrp::Cost;
using pyvrp::CostEvaluator;

Cost CostEvaluator::loadPenalty(Load excessLoad) const
{
    return static_cast<Cost>(excessLoad) * capacityPenalty;
}

Cost CostEvaluator::loadPenalty(Load load, Load capacity) const
{
    return loadPenalty(std::max<Load>(load - capacity, 0));
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp) * timeWarpPenalty;
#endif
}

CostEvaluator::CostEvaluator(Cost capacityPenalty, Cost timeWarpPenalty)
    : capacityPenalty(capacityPenalty), timeWarpPenalty(timeWarpPenalty)
{
}
