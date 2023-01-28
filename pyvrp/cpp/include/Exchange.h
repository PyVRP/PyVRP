#ifndef EXCHANGE_H
#define EXCHANGE_H

#include "LocalSearchOperator.h"
#include "Node.h"

/**
 * Template class that exchanges N consecutive nodes from U's route (starting at
 * U) with M consecutive nodes from V's route (starting at V). As special cases,
 * (1, 0) is pure relocate, and (1, 1) pure swap.
 */
template <size_t N, size_t M> class Exchange : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    static_assert(N >= M && N > 0, "N < M or N == 0 does not make sense");

    // Tests if the segment starting at node of given length contains the depot
    inline bool containsDepot(Node *node, size_t segLength) const;

    // Tests if the segments of U and V overlap in the same route
    inline bool overlap(Node *U, Node *V) const;

    // Tests if the segments of U and V are adjacent in the same route
    inline bool adjacent(Node *U, Node *V) const;

    // Special case that's applied when M == 0
    int evalRelocateMove(Node *U, Node *V) const;

    // Applied when M != 0
    int evalSwapMove(Node *U, Node *V) const;

public:
    int evaluate(Node *U, Node *V) override;

    void apply(Node *U, Node *V) override;
};

#endif  // EXCHANGE_H
