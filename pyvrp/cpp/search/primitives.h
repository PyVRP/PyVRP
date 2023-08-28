#ifndef PYVRP_SEARCH_PRIMITIVES_H
#define PYVRP_SEARCH_PRIMITIVES_H

#include "CostEvaluator.h"
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
 * Evaluates removing U from its current route. The evaluation is exact.
 *
 * Parameters
 * ----------
 * U
 *     Node to remove. Must currently be in a route.
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
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PRIMITIVES_H
