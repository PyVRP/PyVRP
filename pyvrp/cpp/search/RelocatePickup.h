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

    struct Move  // stores cost of reinserting pickup node behind after
    {
        Cost cost = std::numeric_limits<Cost>::max();
        Route::Node const *after = nullptr;
    };

    Move move_;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEPICKUP_H
