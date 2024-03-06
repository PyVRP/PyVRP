#ifndef PYVRP_SEARCH_SWAPTAILS_H
#define PYVRP_SEARCH_SWAPTAILS_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * SwapTails(data: ProblemData)
 *
 * Given two nodes :math:`U` and :math:`V`, tests whether replacing the arc of
 * :math:`U` to its successor :math:`n(U)` and :math:`V` to :math:`n(V)` by
 * :math:`U \rightarrow n(V)` and :math:`V \rightarrow n(U)` is an improving
 * move.
 *
 * .. note::
 *
 *    This operator is also known as 2-OPT* in the VRP literature.
 */
class SwapTails : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPTAILS_H
