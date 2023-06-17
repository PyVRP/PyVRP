#ifndef PYVRP_TWOOPT_H
#define PYVRP_TWOOPT_H

#include "LocalSearchOperator.h"

/**
 * 2-OPT moves.
 *
 * Between routes: replaces U -> X and V -> Y by U -> Y and V -> X, if that is
 * an improving move. Within route: replaces U -> X and V -> Y by U -> V and
 * X -> Y, if that is an improving move.
 */
class TwoOpt : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    Cost
    evalWithinRoute(Node *U, Node *V, CostEvaluator const &costEvaluator) const;

    Cost evalBetweenRoutes(Node *U,
                           Node *V,
                           CostEvaluator const &costEvaluator) const;

    void applyWithinRoute(Node *U, Node *V) const;

    void applyBetweenRoutes(Node *U, Node *V) const;

public:
    Cost
    evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator) override;

    void apply(Node *U, Node *V) const override;
};

#endif  // PYVRP_TWOOPT_H
