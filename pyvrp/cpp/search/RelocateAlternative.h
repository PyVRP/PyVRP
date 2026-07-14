#ifndef PYVRP_SEARCH_RELOCATEWITHALTERNATIVE_H
#define PYVRP_SEARCH_RELOCATEWITHALTERNATIVE_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateAlternative(data: ProblemData)
 *
 * Evaluates replacing the current mutually exclusive group member :math:`U` by
 * another member of the same group, while relocating it after :math:`V`.
 */
class RelocateAlternative : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    Solution *solution_ = nullptr;
    Route::Node *alternative_ = nullptr;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;
};

template <> bool supports<RelocateAlternative>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEWITHALTERNATIVE_H
