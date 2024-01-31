#ifndef PYVRP_REPAIR_NEAREST_ROUTE_INSERT_H
#define PYVRP_REPAIR_NEAREST_ROUTE_INSERT_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "Solution.h"

#include <vector>

namespace pyvrp::repair
{
/**
 * Nearest route insert operator. This operator inserts each client in the list
 * of unplanned clients into one of the given routes. It does so by first
 * determining which route has a center point closest to the client, and then
 * evaluating all possible insert moves of the client into that closest route.
 * The best move is applied. This operator has a quadratic runtime in the worst
 * case, but is typically much more efficient than
 * :func:`~pyvrp.repair._repair.greedy_repair`, at the cost of some solution
 * quality.
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
nearestRouteInsert(std::vector<Solution::Route> const &routes,
                   std::vector<size_t> const &unplanned,
                   ProblemData const &data,
                   CostEvaluator const &costEvaluator);
}  // namespace pyvrp::repair

#endif  // PYVRP_REPAIR_NEAREST_ROUTE_INSERT_H
