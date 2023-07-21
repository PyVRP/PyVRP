#include "RelocateStar.h"

using pyvrp::search::RelocateStar;
using pyvrp::search::Route;

pyvrp::Cost RelocateStar::evaluate(Route *U,
                                   Route *V,
                                   pyvrp::CostEvaluator const &costEvaluator)
{
    move = {};

    for (auto *nodeU : *U)
    {
        // Test inserting U after V's start depot
        Cost deltaCost
            = relocate.evaluate(nodeU, &V->startDepot, costEvaluator);

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, &V->startDepot};

        for (auto *nodeV : *V)
        {
            // Test inserting U after V
            deltaCost = relocate.evaluate(nodeU, nodeV, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            // Test inserting V after U
            deltaCost = relocate.evaluate(nodeV, nodeU, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

void RelocateStar::apply([[maybe_unused]] Route *U,
                         [[maybe_unused]] Route *V) const
{
    move.from->insertAfter(move.to);
}
