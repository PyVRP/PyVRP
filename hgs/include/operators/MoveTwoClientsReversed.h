#ifndef MOVETWOCLIENTSREVERSED_H
#define MOVETWOCLIENTSREVERSED_H

#include "LocalSearchOperator.h"
#include "Node.h"

/**
 * Inserts U -> X after V (as V -> X -> U), if that is an improving move.
 */
class MoveTwoClientsReversed : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    int evaluate(Node *U, Node *V) override;

    void apply(Node *U, Node *V) override
    {
        auto *X = n(U);  // copy since the insert below changes n(U)

        U->insertAfter(V);
        X->insertAfter(V);
    }
};

#endif  // MOVETWOCLIENTSREVERSED_H
