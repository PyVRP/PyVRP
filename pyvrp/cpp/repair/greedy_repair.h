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
 * Greedy repair operator. This operator inserts each client in the set of
 * unplanned clients into the solution. It does so by evaluating all possible
 * moves and applying the best one for each client, resulting in a quadratic
 * runtime.
 *
 * Parameters
 * ----------
 * solution
 *     Solution to repair.
 * unplanned
 *     Set of unplanned clients to insert into the solution.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use when evaluating insertion moves.
 *
 * Returns
 * -------
 * Solution
 *     The repaired solution.
 */
Solution greedyRepair(Solution const &solution,
                      DynamicBitset const &unplanned,
                      ProblemData const &data,
                      CostEvaluator const &costEvaluator);
}  // namespace pyvrp::repair

#endif  // PYVRP_REPAIR_H
