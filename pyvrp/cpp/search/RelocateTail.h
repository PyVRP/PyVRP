#ifndef PYVRP_SEARCH_RELOCATETAIL_H
#define PYVRP_SEARCH_RELOCATETAIL_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateTail(data: ProblemData)
 *
 * TODO
 */
class RelocateTail : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATETAIL_H
