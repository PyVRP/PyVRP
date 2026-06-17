#ifndef PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H
#define PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * InsertOptionalShipment(data: ProblemData)
 *
 * TODO
 */
class InsertOptionalShipment : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <> bool supports<InsertOptionalShipment>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H
