#ifndef PYVRP_SEARCH_RELOCATESHIPMENT_H
#define PYVRP_SEARCH_RELOCATESHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateShipment(data: ProblemData)
 *
 * Relocates the shipment :math:`U` after :math:`V`. :math:`U`'s pickup is
 * relocated after :math:`V`'s pickup, and :math:`U`'s delivery after
 * :math:`V`'s delivery.
 */
class RelocateShipment : public BinaryOperator
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

#endif  // PYVRP_SEARCH_RELOCATESHIPMENT_H
