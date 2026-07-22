#ifndef PYVRP_SEARCH_SWAPSHIPMENT_H
#define PYVRP_SEARCH_SWAPSHIPMENT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * SwapShipment(data: ProblemData)
 *
 * Evaluates whether swapping shipments :math:`U` and :math:`V` in different
 * routes is an improving move.
 */
class SwapShipment : public BinaryOperator
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

#endif  // PYVRP_SEARCH_SWAPSHIPMENT_H
