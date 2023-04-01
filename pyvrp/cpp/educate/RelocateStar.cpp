#include "RelocateStar.h"

cost_type
RelocateStar::evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator)
{
    move = {};

    for (auto *nodeU = n(U->depot); !nodeU->isDepot(); nodeU = n(nodeU))
    {
        // Eval inserting U after V's depot
        auto deltaCost = relocate.evaluate(nodeU, V->depot, costEvaluator);

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, V->depot};

        for (auto *nodeV = n(V->depot); !nodeV->isDepot(); nodeV = n(nodeV))
        {
            // Eval inserting U after V
            deltaCost = relocate.evaluate(nodeU, nodeV, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            // Eval inserting V after U
            deltaCost = relocate.evaluate(nodeV, nodeU, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

void RelocateStar::apply(Route *U, Route *V) const
{
    move.from->insertAfter(move.to);
}
