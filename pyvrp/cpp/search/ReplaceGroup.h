#ifndef PYVRP_SEARCH_REPLACEGROUP_H
#define PYVRP_SEARCH_REPLACEGROUP_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReplaceGroup(data: ProblemData)
 *
 * Evaluates replacing the current mutually exclusive group member :math:`V`
 * with :math:`U`.
 */
class ReplaceGroup : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    Solution *solution_ = nullptr;
    Route::Node *V_ = nullptr;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    void init(Solution &solution) override;
};

template <> bool supports<ReplaceGroup>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACEGROUP_H
