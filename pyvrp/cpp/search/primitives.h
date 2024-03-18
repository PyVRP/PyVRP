#ifndef PYVRP_SEARCH_PRIMITIVES_H
#define PYVRP_SEARCH_PRIMITIVES_H

#include "CostEvaluator.h"
#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "Measure.h"
#include "Route.h"

#include <tuple>

// This file stores a few basic functions for (precisely) evaluating really
// common moves. Those primitives may be useful implementing higher order
// operators.
namespace pyvrp::search
{
template <typename... Segments> struct RouteProposal
{
    ProblemData const &data;
    std::tuple<Segments...> segments;

    RouteProposal(ProblemData const &data, Segments &&...segments)
        : data(data), segments(std::forward<Segments>(segments)...)
    {
    }

    DistanceSegment distanceSegment() const
    {
        return std::apply(
            [&](auto &&...args) {
                return DistanceSegment::merge(data.distanceMatrix(), args...);
            },
            segments);
    }

    DurationSegment durationSegment() const
    {
        return std::apply(
            [&](auto &&...args) {
                return DurationSegment::merge(data.durationMatrix(), args...);
            },
            segments);
    }

    LoadSegment loadSegment() const
    {
        return std::apply(
            [](auto &&...args) { return LoadSegment::merge(args...); },
            segments);
    }
};

/**
 * TODO
 */
template <typename... Args>
Cost deltaCost(Route *U,
               RouteProposal<Args...> const &prop,
               CostEvaluator const &costEvaluator);

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
pyvrp::Cost pyvrp::search::deltaCost(Route *U,
                                     RouteProposal<Args...> const &prop,
                                     CostEvaluator const &costEvaluator)
{
    Cost deltaCost = 0;

    auto const distSegment = prop.distanceSegment();

    deltaCost += static_cast<Cost>(distSegment.distance());
    deltaCost -= static_cast<Cost>(U->distance());

    deltaCost
        += costEvaluator.distPenalty(distSegment.distance(), U->maxDistance());
    deltaCost -= costEvaluator.distPenalty(U->distance(), U->maxDistance());

    deltaCost -= costEvaluator.loadPenalty(U->load(), U->capacity());
    deltaCost -= costEvaluator.twPenalty(U->timeWarp());

    if (deltaCost >= 0)
        return deltaCost;

    auto const loadSegment = prop.loadSegment();
    auto const durationSegment = prop.durationSegment();

    deltaCost += costEvaluator.loadPenalty(loadSegment.load(), U->capacity());
    deltaCost
        += costEvaluator.twPenalty(durationSegment.timeWarp(U->maxDuration()));

    return deltaCost;
}

#endif  // PYVRP_SEARCH_PRIMITIVES_H
