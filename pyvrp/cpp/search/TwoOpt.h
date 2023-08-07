#ifndef PYVRP_TWOOPT_H
#define PYVRP_TWOOPT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * TwoOpt(data: ProblemData)
 *
 * Given two clients :math:`U` and :math:`V`, tests:
 *
 * * If :math:`U` and :math:`V` are not in the same route, tests replacing the
 *   arc of :math:`U` to its successor :math:`n(U)` and :math:`V` to
 *   :math:`n(V)` by :math:`U \rightarrow n(V)` and :math:`V \rightarrow n(U)`.
 * * If :math:`U` and :math:`V` are in the same route, tests replacing
 *   :math:`U \rightarrow n(U)` and :math:`V \rightarrow n(V)` by
 *   :math:`U \rightarrow V` and :math:`n(U) \rightarrow n(V)`. This reverses
 *   the route segment from :math:`n(U)` to :math:`V`.
 */
class TwoOpt : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    Cost evalWithinRoute(Route::Node *U,
                         Route::Node *V,
                         CostEvaluator const &costEvaluator) const;

    Cost evalBetweenRoutes(Route::Node *U,
                           Route::Node *V,
                           CostEvaluator const &costEvaluator) const;

    void applyWithinRoute(Route::Node *U, Route::Node *V) const;

    void applyBetweenRoutes(Route::Node *U, Route::Node *V) const;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_TWOOPT_H
