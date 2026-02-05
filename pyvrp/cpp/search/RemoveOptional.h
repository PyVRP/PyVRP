#ifndef PYVRP_SEARCH_REMOVEOPTIONAL_H
#define PYVRP_SEARCH_REMOVEOPTIONAL_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RemoveOptional(data: ProblemData)
 *
 * Evaluates removing an optional client node :math:`U`.
 */
class RemoveOptional : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    Solution const *solution_ = nullptr;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    void init(Solution const &solution) override;
};

template <> bool supports<RemoveOptional>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REMOVEOPTIONAL_H
