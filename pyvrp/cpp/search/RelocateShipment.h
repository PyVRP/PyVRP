#ifndef PYVRP_SEARCH_RELOCATESHIPMENT_H
#define PYVRP_SEARCH_RELOCATESHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateShipment(data: ProblemData)
 *
 * Evaluates relocating the shipment :math:`U` after :math:`V`. :math:`U` is
 * relocated directly after :math:`V`, and :math:`U`'s delivery is inserted in
 * the first improving place after :math:`U`.
 */
class RelocateShipment : public BinaryOperator
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

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATESHIPMENT_H
