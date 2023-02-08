#ifndef MOVETWOCLIENTSREVERSED_H
#define MOVETWOCLIENTSREVERSED_H

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
 * Inserts U -> X after V (as V -> X -> U), if that is an improving move.
 */
class MoveTwoClientsReversed : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    TCost evaluate(Node *U, Node *V) override;

    void apply(Node *U, Node *V) override;
};

#endif  // MOVETWOCLIENTSREVERSED_H
