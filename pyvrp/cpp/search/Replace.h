#ifndef PYVRP_SEARCH_REPLACE_H
#define PYVRP_SEARCH_REPLACE_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{

/**
 * Replace(data: ProblemData)
 *
 * Given two nodes :math:`U` and :math:`V`, where :math:`U` is in a route and
 * :math:`V` is not, this operator tests whether replacing :math:`V` for
 * :math:`U` is an improving move.
 */
class Replace : public NodeOperator
{
    using NodeOperator::NodeOperator;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <> bool supports<Replace>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACE_H
