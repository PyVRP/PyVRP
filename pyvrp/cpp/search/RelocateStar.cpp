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
        // Test inserting U after V's depot (idx 0).
        auto *depot = (*V)[0];
        Cost deltaCost = relocate.evaluate(nodeU, depot, costEvaluator);

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, depot};

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
    auto *fromRoute = move.from->route();
    auto *toRoute = move.to->route();

    fromRoute->remove(move.from->idx());
    toRoute->insert(move.to->idx() + 1, move.from);
}
