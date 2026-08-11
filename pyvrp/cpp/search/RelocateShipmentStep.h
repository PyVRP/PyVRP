#ifndef PYVRP_SEARCH_RELOCATESHIPMENTSTEP_H
#define PYVRP_SEARCH_RELOCATESHIPMENTSTEP_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateShipmentStep(data: ProblemData)
 *
 * Evaluates relocating either step of the shipment represented by pickup node
 * :math:`U` to an improving position in its current route.
 */
class RelocateShipmentStep : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    struct Move
    {
        Cost cost = std::numeric_limits<Cost>::max();
        Route::Node *step = nullptr;
        Route::Node const *before = nullptr;
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

#endif  // PYVRP_SEARCH_RELOCATESHIPMENTSTEP_H
