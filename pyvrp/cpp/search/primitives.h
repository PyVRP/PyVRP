#ifndef PYVRP_SEARCH_PRIMITIVES_H
#define PYVRP_SEARCH_PRIMITIVES_H

#include "CostEvaluator.h"
#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "Measure.h"
#include "Route.h"

// This file stores a few basic functions for (precisely) evaluating really
// common moves. Those primitives may be useful implementing higher order
// operators.
namespace pyvrp::search
{
/**
 * TODO
 */
template <typename... Args>
Cost deltaCost(Route *route,
               ProblemData const &data,
               CostEvaluator const &costEvaluator,
               Args &&...args);

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

template <typename... Args>
pyvrp::Cost pyvrp::search::deltaCost(Route *route,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator,
                                     Args &&...args)
{
    Cost deltaCost = 0;

    auto const distSegment
        = DistanceSegment::merge(data.distanceMatrix(), args...);

    deltaCost += static_cast<Cost>(distSegment.distance());
    deltaCost -= static_cast<Cost>(route->distance());

    deltaCost += costEvaluator.distPenalty(distSegment.distance(),
                                           route->maxDistance());
    deltaCost
        -= costEvaluator.distPenalty(route->distance(), route->maxDistance());

    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());
    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    if (deltaCost >= 0)
        return deltaCost;

    auto const loadSegment = LoadSegment::merge(args...);

    deltaCost
        += costEvaluator.loadPenalty(loadSegment.load(), route->capacity());

    auto const durationSegment
        = DurationSegment::merge(data.durationMatrix(), args...);

    deltaCost += costEvaluator.twPenalty(
        durationSegment.timeWarp(route->maxDuration()));

    return deltaCost;
}

#endif  // PYVRP_SEARCH_PRIMITIVES_H
