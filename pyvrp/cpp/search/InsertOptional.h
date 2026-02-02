#ifndef PYVRP_SEARCH_INSERTOPTIONAL_H
#define PYVRP_SEARCH_INSERTOPTIONAL_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * InsertOptional(data: ProblemData)
 *
 * Evaluates inserting an optional client node :math:`U` behind :math:`V`.
 */
class InsertOptional : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <> bool supports<InsertOptional>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERTOPTIONAL_H
