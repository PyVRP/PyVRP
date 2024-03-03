#ifndef PYVRP_SEARCH_REVERSESEGMENT_H
#define PYVRP_SEARCH_REVERSESEGMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReverseSegment(data: ProblemData)
 *
 * Given two clients :math:`U` and :math:`V` in the same route, tests replacing
 * :math:`U \rightarrow n(U)` and :math:`V \rightarrow n(V)` by
 * :math:`U \rightarrow V` and :math:`n(U) \rightarrow n(V)`. This reverses the
 * route segment from :math:`n(U)` to :math:`V`.
 *
 * .. note::
 *
 *    This operator is equivalent to a 2-OPT move for TSP.
 */
class ReverseSegment : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REVERSESEGMENT_H
