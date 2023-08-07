#ifndef PYVRP_MOVETWOCLIENTSREVERSED_H
#define PYVRP_MOVETWOCLIENTSREVERSED_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * MoveTwoClientsReversed(data: ProblemData)
 *
 * Given two clients :math:`U` and :math:`V`, tests if inserting :math:`U` and
 * its successor :math:`n(U)` after :math:`V` as
 * :math:`V \rightarrow n(U) \rightarrow U` is an improving move.
 */
class MoveTwoClientsReversed : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_MOVETWOCLIENTSREVERSED_H
