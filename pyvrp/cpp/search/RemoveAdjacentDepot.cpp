#include "RemoveAdjacentDepot.h"

#include "primitives.h"

#include <cassert>
#include <limits>

using pyvrp::search::RemoveAdjacentDepot;

std::pair<pyvrp::Cost, bool>
RemoveAdjacentDepot::evaluate(Route::Node *U,
                              CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot());
    stats_.numEvaluations++;

    if (!U->route())
        return std::make_pair(0, false);

    Cost bestCost = std::numeric_limits<Cost>::max();

    if (p(U)->isReloadDepot())
    {
        Cost deltaCost = removeCost(p(U), data, costEvaluator);
        if (deltaCost < bestCost)
        {
            bestCost = deltaCost;
            move_ = MoveType::REMOVE_PREV;
        }
    }

    if (n(U)->isReloadDepot())
    {
        Cost deltaCost = removeCost(n(U), data, costEvaluator);
        if (deltaCost < bestCost)
        {
            bestCost = deltaCost;
            move_ = MoveType::REMOVE_NEXT;
        }
    }

    // Apply this move it's either better or neutral. It can be neutral if e.g.
    // the same depot is visited consecutively, but that's unnecessary.
    return std::make_pair(bestCost, bestCost <= 0);
}

void RemoveAdjacentDepot::apply(Route::Node *U) const
{
    stats_.numApplications++;
    auto *route = U->route();

    if (move_ == MoveType::REMOVE_PREV)
        route->remove(U->idx() - 1);

    if (move_ == MoveType::REMOVE_NEXT)
        route->remove(U->idx() + 1);
}

template <>
bool pyvrp::search::supports<RemoveAdjacentDepot>(ProblemData const &data)
{
    // We need at least one vehicle type for which reloading is enabled.
    for (auto const &vehType : data.vehicleTypes())
        if (!vehType.reloadDepots.empty() && vehType.maxReloads != 0)
            return true;

    return false;
}
