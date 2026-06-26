#ifndef PYVRP_SEARCH_REPLACEOPTIONALCLIENT_H
#define PYVRP_SEARCH_REPLACEOPTIONALCLIENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReplaceOptionalClient(data: ProblemData)
 *
 * Evaluates replacing an optional client node :math:`V` with :math:`U`.
 */
class ReplaceOptionalClient : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    std::string name() const override;
};

template <> bool supports<ReplaceOptionalClient>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACEOPTIONALCLIENT_H
