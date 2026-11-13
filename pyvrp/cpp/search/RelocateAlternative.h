#ifndef PYVRP_SEARCH_RELOCATEALTERNATIVE_H
#define PYVRP_SEARCH_RELOCATEALTERNATIVE_H

#include "DynamicBitset.h"
#include "LocalSearchOperator.h"

#include <vector>

namespace pyvrp::search
{
/**
 * RelocateAlternative(data: ProblemData)
 *
 * Evaluates replacing mutually exclusive group member :math:`U` with another
 * member of the same group and relocating the replacement after :math:`V`.
 *
 * The replacement client or pickup is always inserted after :math:`V`. When
 * replacing a shipment within the same route, the delivery stays in place if
 * it follows the new pickup; otherwise, it is inserted directly after the new
 * pickup. For other shipment alternatives, the delivery is inserted in the
 * first improving position after the pickup in the same trip. Group members
 * are evaluated in order until an improving move is found.
 * Replacements in place are handled by ``ReplaceGroup``.
 */
class RelocateAlternative : public BinaryOperator
{
    struct Move
    {
        Cost cost = 0;
        Route::Node *alternative = nullptr;
        Route::Node const *deliveryAfter = nullptr;
    };

    Move move_;
    Solution *solution_ = nullptr;

    DynamicBitset hasCachedRemoveCost_;
    std::vector<Cost> removeCost_;

    // Same-route insertions. In-place replacements are handled by ReplaceGroup.
    Cost evalClientToClient(Route::Node const *U,
                            Route::Node const *V,
                            size_t alternative,
                            Cost deltaCost,
                            CostEvaluator const &costEvaluator) const;

    Cost evalShipmentToClient(Route::Node const *U,
                              Route::Node const *V,
                              size_t alternative,
                              Cost deltaCost,
                              CostEvaluator const &costEvaluator) const;

    Cost evalClientToShipment(Route::Node const *U,
                              Route::Node const *V,
                              size_t alternative,
                              Route::Node const *deliveryAfter,
                              Cost deltaCost,
                              CostEvaluator const &costEvaluator) const;

    // Keeps delivery in place when possible, otherwise inserts it after pickup.
    Cost evalShipmentToShipment(Route::Node const *U,
                                Route::Node const *V,
                                size_t alternative,
                                Cost deltaCost,
                                CostEvaluator const &costEvaluator) const;

    // Evaluates relocation moves when U and V are in the same route.
    void evalSameRoute(Route::Node *U,
                       Route::Node *V,
                       CostEvaluator const &costEvaluator);

    // Evaluates relocation moves when U and V are in different routes.
    void evalDifferentRoutes(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;

    static bool supports(ProblemData const &data);

    void update(Route const *route) override;

    RelocateAlternative(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEALTERNATIVE_H
