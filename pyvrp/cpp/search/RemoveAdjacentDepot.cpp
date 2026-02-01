#include "RemoveAdjacentDepot.h"

#include "primitives.h"

#include <cassert>
#include <limits>

using pyvrp::search::RemoveAdjacentDepot;

pyvrp::Cost RemoveAdjacentDepot::evaluate(Route::Node *U,
                                          CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot());
    stats_.numEvaluations++;

    if (!U->route())
        return 0;

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

    return bestCost;
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
