#ifndef RELOCATESTAR_H
#define RELOCATESTAR_H

#include "Exchange.h"
#include "LocalSearchOperator.h"
#include "Node.h"
#include "Route.h"

/**
 * Performs the best (1, 0)-exchange move between routes U and V. Tests both
 * ways: from U to V, and from V to U.
 */
class RelocateStar : public LocalSearchOperator<Route>
{
    struct Move
    {
        TCost deltaCost = 0;
        Node *from = nullptr;
        Node *to = nullptr;
    };

    Exchange<1, 0> relocate;
    Move move;

public:
    TCost evaluate(Route *U, Route *V) override;

    void apply(Route *U, Route *V) override;

    explicit RelocateStar(ProblemData const &data,
                          PenaltyManager const &penaltyManager)
        : LocalSearchOperator<Route>(data, penaltyManager),
          relocate(data, penaltyManager)
    {
    }
};

#endif  // RELOCATESTAR_H
