#ifndef PYVRP_SEARCH_TRIPRELOCATE_H
#define PYVRP_SEARCH_TRIPRELOCATE_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * TripRelocate(data: ProblemData)
 *
 * Tests a few moves while relocating :math:`U` after :math:`V`:
 *
 * * If :math:`U` borders a reload depot, tests if moving the reload depot
 *   along with :math:`U` is better;
 * * If :math:`V` or :math:`n(V)` is not a depot, tests if inserting a reload
 *   depot after or before :math:`U` is better.
 */
class TripRelocate : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_TRIPRELOCATE_H
