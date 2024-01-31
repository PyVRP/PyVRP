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
 * unplanned clients into the given routes. It does so by evaluating all
 * possible moves and applying the best one for each client, resulting in a
 * quadratic runtime.
 *
 * Parameters
 * ----------
 * routes
 *     List of routes.
 * unplanned
 *     Unplanned clients to insert into the routes.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use when evaluating insertion moves.
 *
 * Returns
 * -------
 * list[Route]
 *     The list of repaired routes.
 *
 * Raises
 * ------
 * ValueError
 *     When the list of routes is empty but the list of unplanned clients is
 *     not.
 */
std::vector<Solution::Route>
greedyRepair(std::vector<Solution::Route> const &routes,
             std::vector<size_t> const &unplanned,
             ProblemData const &data,
             CostEvaluator const &costEvaluator);
}  // namespace pyvrp::repair

#endif  // PYVRP_GREEDY_REPAIR_H
