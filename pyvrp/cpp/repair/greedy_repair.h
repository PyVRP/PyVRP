#ifndef PYVRP_GREEDY_REPAIR_H
#define PYVRP_GREEDY_REPAIR_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "Solution.h"

#include <vector>

namespace pyvrp::repair
{
/**
 * Greedy repair operator. This operator inserts each client in the list of
 * unplanned clients into the solution. It does so by evaluating all possible
 * moves and applying the best one for each client, resulting in a quadratic
 * runtime.
 *
 * Parameters
 * ----------
 * solution
 *     Solution to repair.
 * unplanned
 *     Unplanned clients to insert into the solution.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use when evaluating insertion moves.
 *
 * Returns
 * -------
 * Solution
 *     The repaired solution.
 *
 * Raises
 * ------
 * ValueError
 *     When the solution is empty but the list of unplanned clients is not.
 */
Solution greedyRepair(Solution const &solution,
                      std::vector<size_t> const &unplanned,
                      ProblemData const &data,
                      CostEvaluator const &costEvaluator);

// Convenient alternative overload.
Solution greedyRepair(std::vector<Solution::Route> const &routes,
                      std::vector<size_t> const &unplanned,
                      ProblemData const &data,
                      CostEvaluator const &costEvaluator);
}  // namespace pyvrp::repair

#endif  // PYVRP_GREEDY_REPAIR_H
