#include "InsertOptionalShipment.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <cassert>

using pyvrp::search::InsertOptionalShipment;

std::pair<pyvrp::Cost, bool> InsertOptionalShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(!V->isEndDepot());
    stats_.numEvaluations++;

    if (!U->isShipment() || U->route() || !V->route())
        return std::make_pair(0, false);

    move_ = {};

    auto const &shipment = data.shipment(U->idx());
    auto const *route = V->route();

    if (U->isPickup())  // pickup after V, delivery later in the route
        for (size_t pos = V->pos() + 1; pos != route->size() - 1; ++pos)
        {
            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(V->pos()),
                                PickupSegment(data, U->idx()),
                                route->between(V->pos() + 1, pos),
                                DeliverySegment(data, U->idx()),
                                route->after(pos + 1)));

            if (deltaCost < 0)
            {
                move_ = {pos};
                return std::make_pair(deltaCost, true);
            }
        }
    else if (!V->isStartDepot())  // delivery after V, pickup earlier in route
        for (size_t pos = 1; pos != V->pos(); ++pos)
        {
            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(pos - 1),
                                PickupSegment(data, U->idx()),
                                route->between(pos, V->pos()),
                                DeliverySegment(data, U->idx()),
                                route->after(V->pos() + 1)));

            if (deltaCost < 0)
            {
                move_ = {pos};
                return std::make_pair(deltaCost, true);
            }
        }

    return std::make_pair(0, false);
}

void InsertOptionalShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isShipment() && !U->route() && V->route());
    assert(V->pos() < move_.pos);
    stats_.numApplications++;

    auto *route = V->route();

    if (U->isPickup())
    {
        route->insert(move_.pos, U + 1);
        route->insert(V->pos(), U);
    }
    else
    {
        route->insert(move_.pos, U - 1);
        route->insert(V->pos(), U);
    }
}

template <>
bool pyvrp::search::supports<InsertOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
