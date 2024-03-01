#ifndef PYVRP_SEARCH_SWAPTAILS_H
#define PYVRP_SEARCH_SWAPTAILS_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * SwapTails(data: ProblemData)
 *
 * Given two routes :math:`U` and :math:`V`, tests for each client :math:`C_U`
 * in :math:`U` and :math:`C_V` in :math:`V` whether replacing the arc of
 * :math:`C_U` to its successor :math:`n(C_U)` and :math:`C_V` to :math:`n(C_V)`
 * by :math:`C_U \rightarrow n(C_V)` and :math:`C_V \rightarrow n(C_U)` is an
 * improving move. The best move is stored and may be applied.
 */
class SwapTails : public LocalSearchOperator<Route>
{
    using LocalSearchOperator::LocalSearchOperator;

    struct Move
    {
        Cost deltaCost = 0;
        Route::Node *U = nullptr;
        Route::Node *V = nullptr;
    };

    Move move;

    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator);

public:
    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPTAILS_H
