#ifndef PYVRP_SEARCH_RELOCATEDELIVERY_H
#define PYVRP_SEARCH_RELOCATEDELIVERY_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateDelivery(data: ProblemData)
 *
 * Evaluates relocating the delivery node of the shipment represented by
 * pickup node :math:`U` to the first improving position in its current trip.
 */
class RelocateDelivery : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    struct Move
    {
        Route::Node const *after = nullptr;
    };

    Move move_;

    // Evaluate placing the delivery node after its current position.
    Cost evalAfter(Route::Node *U, CostEvaluator const &costEvaluator);

    // Evaluate placing the delivery node before its current position.
    Cost evalBefore(Route::Node *U, CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEDELIVERY_H
