#ifndef PYVRP_REPAIR_H
#define PYVRP_REPAIR_H

#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "ProblemData.h"
#include "Solution.h"

#include <vector>

namespace pyvrp::repair
{
/**
 * TODO
 */
Solution greedyRepair(Solution const &solution,
                      DynamicBitset const &unplanned,
                      ProblemData const &data,
                      CostEvaluator const &costEvaluator);
}  // namespace pyvrp::repair

#endif  // PYVRP_REPAIR_H
