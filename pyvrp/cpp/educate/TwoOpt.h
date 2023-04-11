#ifndef TWOOPT_H
#define TWOOPT_H

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

    int
    evalWithinRoute(Node *U, Node *V, CostEvaluator const &costEvaluator) const;

    int evalBetweenRoutes(Node *U,
                          Node *V,
                          CostEvaluator const &costEvaluator) const;

    void applyWithinRoute(Node *U, Node *V) const;

    void applyBetweenRoutes(Node *U, Node *V) const;

public:
    int evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator) override;

    void apply(Node *U, Node *V) const override;
};

#endif  // TWOOPT_H
