#ifndef PYVRP_SEARCH_RELOCATESHIPMENT_H
#define PYVRP_SEARCH_RELOCATESHIPMENT_H

#include "DynamicBitset.h"
#include "LocalSearchOperator.h"

#include <vector>

namespace pyvrp::search
{
/**
 * RelocateShipment(data: ProblemData)
 *
 * Evaluates relocating the shipment :math:`U` after :math:`V` in different
 * routes. :math:`U` is relocated directly after :math:`V`, and :math:`U`'s
 * delivery is inserted in the first improving place after :math:`U`.
 */
class RelocateShipment : public BinaryOperator
{
    struct Move
    {
        size_t pos = 0;
    };

    Move move_;

    DynamicBitset hasCachedRemoveCost_;
    std::vector<Cost> removeCost_;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;

    static bool supports(ProblemData const &data);

    void update(Route const *route) override;

    RelocateShipment(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATESHIPMENT_H
