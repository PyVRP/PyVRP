#ifndef PYVRP_SEARCH_REMOVEOPTIONALCLIENT_H
#define PYVRP_SEARCH_REMOVEOPTIONALCLIENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RemoveOptionalClient(data: ProblemData)
 *
 * Evaluates removing an optional client node :math:`U`.
 */
class RemoveOptionalClient : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;
};

template <> bool supports<RemoveOptionalClient>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REMOVEOPTIONALCLIENT_H
