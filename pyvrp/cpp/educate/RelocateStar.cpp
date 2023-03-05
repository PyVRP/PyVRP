#include "RelocateStar.h"

cost_type RelocateStar::evaluate(Route *U, Route *V)
{
    move = {};

    for (auto *nodeU = n(U->depot); !nodeU->isDepot(); nodeU = n(nodeU))
    {
        auto deltaCost = relocate.evaluate(nodeU, V->depot);  // eval depot

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, V->depot};

        for (auto *nodeV = n(V->depot); !nodeV->isDepot(); nodeV = n(nodeV))
        {
            deltaCost = relocate.evaluate(nodeU, nodeV);  // eval U after V

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            deltaCost = relocate.evaluate(nodeV, nodeU);  // eval V after U

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

void RelocateStar::apply(Route *U, Route *V)
{
    move.from->insertAfter(move.to);
}
