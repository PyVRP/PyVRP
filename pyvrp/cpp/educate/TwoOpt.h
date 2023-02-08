#ifndef TWOOPT_H
#define TWOOPT_H

#include "LocalSearchOperator.h"
#include "Node.h"

#ifdef INT_PRECISION
using TCost = int;
using TDist = int;
using TTime = int;
#else
using TCost = double;
using TDist = double;
using TTime = double;
#endif

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

    TCost evalWithinRoute(Node *U, Node *V);

    TCost evalBetweenRoutes(Node *U, Node *V);

    void applyWithinRoute(Node *U, Node *V);

    void applyBetweenRoutes(Node *U, Node *V);

public:
    TCost evaluate(Node *U, Node *V) override;

    void apply(Node *U, Node *V) override;
};

#endif  // TWOOPT_H
