#ifndef PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H
#define PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * InsertOptionalShipment(data: ProblemData)
 *
 * Evaluates inserting the optional shipment :math:`U` after :math:`V`. The
 * node :math:`U` is placed directly after :math:`V`, the other node of the
 * shipment pair is inserted in the first improving location of :math:`V`'s
 * route.
 */
class InsertOptionalShipment : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    struct Move
    {
        size_t pos = 0;
    };

    Move move_;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    std::string name() const override;
};

template <> bool supports<InsertOptionalShipment>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERTOPTIONALSHIPMENT_H
