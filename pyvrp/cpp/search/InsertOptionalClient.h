#ifndef PYVRP_SEARCH_INSERTOPTIONALCLIENT_H
#define PYVRP_SEARCH_INSERTOPTIONALCLIENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * InsertOptionalClient(data: ProblemData)
 *
 * Evaluates inserting an optional client node :math:`U` after :math:`V`.
 */
class InsertOptionalClient : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    Solution const *solution_ = nullptr;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;
};

template <> bool supports<InsertOptionalClient>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERTOPTIONALCLIENT_H
