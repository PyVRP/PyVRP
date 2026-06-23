#ifndef PYVRP_SEARCH_REMOVEOPTIONALSHIPMENT_H
#define PYVRP_SEARCH_REMOVEOPTIONALSHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RemoveOptionalShipment(data: ProblemData)
 *
 * Evaluates removing the optional shipment :math:`U`.
 */
class RemoveOptionalShipment : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;
};

template <> bool supports<RemoveOptionalShipment>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REMOVEOPTIONALSHIPMENT_H
