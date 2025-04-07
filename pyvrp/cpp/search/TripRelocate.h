#ifndef PYVRP_SEARCH_TRIPRELOCATE_H
#define PYVRP_SEARCH_TRIPRELOCATE_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * TripRelocate(data: ProblemData)
 *
 * Tests if inserting a reload depot while relocating :math:`U` after :math:`V`
 * results in an improving move.
 */
class TripRelocate : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    enum class MoveType
    {
        DEPOT_U,  // V -> depot -> U
        U_DEPOT,  // V -> U -> depot
    };

    struct Move
    {
        Cost cost = std::numeric_limits<Cost>::max();
        MoveType type = MoveType::DEPOT_U;
        size_t depot = 0;
    };

    Move move_;

    // Evaluates moves where a reload depot is inserted before U, as
    // V -> depot -> U.
    void evalDepotBefore(Cost &deltaCost,
                         Route::Node *U,
                         Route::Node *V,
                         CostEvaluator const &costEvaluator);

    // Evaluates moves where a reload depot is inserted after U, as
    // V -> U -> depot.
    void evalDepotAfter(Cost &deltaCost,
                        Route::Node *U,
                        Route::Node *V,
                        CostEvaluator const &costEvaluator);

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_TRIPRELOCATE_H
