#ifndef PYVRP_TWOOPT_H
#define PYVRP_TWOOPT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * 2-OPT moves.
 *
 * Between routes: replaces U -> X and V -> Y by U -> Y and V -> X, if that is
 * an improving move. Within route: replaces U -> X and V -> Y by U -> V and
 * X -> Y, if that is an improving move.
 */
class TwoOpt : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    Cost evalWithinRoute(Route::Node *U,
                         Route::Node *V,
                         CostEvaluator const &costEvaluator) const;

    Cost evalBetweenRoutes(Route::Node *U,
                           Route::Node *V,
                           CostEvaluator const &costEvaluator) const;

    void applyWithinRoute(Route::Node *U, Route::Node *V) const;

    void applyBetweenRoutes(Route::Node *U, Route::Node *V) const;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};
}  // namespace pyvrp::search

#endif  // PYVRP_TWOOPT_H
