#ifndef TWOOPT_H
#define TWOOPT_H

#include "LocalSearchOperator.h"
#include "Node.h"

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

    cost_type evalWithinRoute(Node *U, Node *V);

    cost_type evalBetweenRoutes(Node *U, Node *V);

    void applyWithinRoute(Node *U, Node *V);

    void applyBetweenRoutes(Node *U, Node *V);

public:
    cost_type evaluate(Node *U, Node *V) override;

    void apply(Node *U, Node *V) override;
};

#endif  // TWOOPT_H
