#ifndef PYVRP_SEARCH_RELOCATEPICKUP_H
#define PYVRP_SEARCH_RELOCATEPICKUP_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocatePickup(data: ProblemData)
 *
 * Evaluate relocating the pickup node :math:`U` to the best position in its
 * current route.
 */
class RelocatePickup : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    struct Move
    {
        Route::Node const *before = nullptr;
    };

    Move move_;

    // Evaluate placing the pickup node after its current position.
    Cost evalAfter(Route::Node *U, CostEvaluator const &costEvaluator);

    // Evaluate placing the pickup node before its current position.
    Cost evalBefore(Route::Node *U, CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEPICKUP_H
