#ifndef PYVRP_SEARCH_REPLACEOPTIONALSHIPMENT_H
#define PYVRP_SEARCH_REPLACEOPTIONALSHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReplaceOptionalShipment(data: ProblemData)
 *
 * TODO
 */
class ReplaceOptionalShipment : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <> bool supports<ReplaceOptionalShipment>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACEOPTIONALSHIPMENT_H
