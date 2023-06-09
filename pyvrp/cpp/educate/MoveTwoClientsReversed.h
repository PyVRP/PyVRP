#ifndef PYVRP_MOVETWOCLIENTSREVERSED_H
#define PYVRP_MOVETWOCLIENTSREVERSED_H

#include "LocalSearchOperator.h"

/**
 * Inserts U -> X after V (as V -> X -> U), if that is an improving move.
 */
class MoveTwoClientsReversed : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

public:
    Cost
    evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator) override;

    void apply(Node *U, Node *V) const override;
};

#endif  // PYVRP_MOVETWOCLIENTSREVERSED_H
