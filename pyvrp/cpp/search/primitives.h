#ifndef PYVRP_SEARCH_PRIMITIVES_H
#define PYVRP_SEARCH_PRIMITIVES_H

#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "Route.h"

// This file stores a few basic functions for (precisely) evaluating really
// common moves. Those primitives may be useful implementing higher order
// operators.
namespace pyvrp::search
{
/**
 * Evaluates the delta cost of inserting U after V in V's route. The evaluation
 * is exact.
 *
 * Parameters
 * ----------
 * U
 *     Node to insert.
 * V
 *     Node to insert U after. Must be in a route.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use.
 *
 * Returns
 * -------
 * int
 *     Exact delta cost of inserting U after V.
 */
Cost insertCost(Route::Node *U,
                Route::Node *V,
                ProblemData const &data,
                CostEvaluator const &costEvaluator);

/**
 * Evaluates the delta cost of inserting U in the place of V. The evaluation is
 * exact.
 *
 * Parameters
 * ----------
 * U
 *     Node to insert. Must not be in a route.
 * V
 *     Node to insert U in place of. Must be in a route.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use.
 *
 * Returns
 * -------
 * int
 *     Exact delta cost of inserting U in place of V.
 */
Cost inplaceCost(Route::Node *U,
                 Route::Node *V,
                 ProblemData const &data,
                 CostEvaluator const &costEvaluator);

/**
 * Evaluates removing U from its current route. The evaluation is exact.
 *
 * Parameters
 * ----------
 * U
 *     Node to remove (client or reload depot). Must currently be in a route.
 * data
 *     Problem data instance.
 * cost_evaluator
 *     Cost evaluator to use.
 *
 * Returns
 * -------
 * int
 *     Exact delta cost of removing U.
 */
Cost removeCost(Route::Node *U,
                ProblemData const &data,
                CostEvaluator const &costEvaluator);

/**
 * Finds the best insertion position for U among its neighbors and empty routes.
 *
 * Parameters
 * ----------
 * U
 *     Node to insert. Must not be in a route.
 * data
 *     Problem data instance.
 * costEvaluator
 *     Cost evaluator to use.
 * neighbours
 *     Neighbourhood structure of each client.
 * nodes
 *     Nodes in the solution.
 * routes
 *     Routes in the solution.
 *
 * Returns
 * -------
 * std::pair<Route::Node *, Cost>
 *     Best node to insert after and its insertion cost. If no insertion
 *     position was found, returns the start depot of the first route and
 *     a large cost.
 */
std::pair<Route::Node *, Cost>
bestInsert(Route::Node *U,
           ProblemData const &data,
           CostEvaluator const &costEvaluator,
           std::vector<std::vector<size_t>> const &neighbours,
           std::vector<Route::Node> &nodes,
           std::vector<Route> &routes);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PRIMITIVES_H
