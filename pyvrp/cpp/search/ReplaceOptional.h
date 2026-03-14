#ifndef PYVRP_SEARCH_REPLACEOPTIONAL_H
#define PYVRP_SEARCH_REPLACEOPTIONAL_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReplaceOptional(data: ProblemData)
 *
 * Evaluates replacing an optional client node :math:`V` with :math:`U`.
 */
class ReplaceOptional : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <> bool supports<ReplaceOptional>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACEOPTIONAL_H
