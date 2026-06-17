#include "ReplaceOptionalShipment.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <cassert>

using pyvrp::search::ReplaceOptionalShipment;

std::pair<pyvrp::Cost, bool> ReplaceOptionalShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (U->route() || !V->route() || !U->isShipment() || !V->isShipment())
        return std::make_pair(0, false);

    assert(U->isShipment() && V->isShipment());
    auto const &uShipment = data.shipment(U->idx());
    auto const &vShipment = data.shipment(V->idx());

    if (vShipment.required)
        return std::make_pair(0, false);

    Route::Node *vPickup, *vDelivery;
    if (V->isPickup())
    {
        vPickup = V;
        vDelivery = V + 1;
    }
    else
    {
        vDelivery = V;
        vPickup = V - 1;
    }

    assert(vPickup->pos() < vDelivery->pos());
    auto const *route = V->route();

    Cost deltaCost = vShipment.prize - uShipment.prize;
    if (n(vPickup) == vDelivery)
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(route->before(vPickup->pos() - 1),
                            PickupSegment(data, U->idx()),
                            DeliverySegment(data, U->idx()),
                            route->after(vDelivery->pos() + 1)));
    else
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(
                route->before(vPickup->pos() - 1),
                PickupSegment(data, U->idx()),
                route->between(vPickup->pos() + 1, vDelivery->pos() - 1),
                DeliverySegment(data, U->idx()),
                route->after(vDelivery->pos() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void ReplaceOptionalShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isShipment() && V->isShipment() && V->route() && !U->route());
    stats_.numApplications++;

    Route::Node *uPickup, *uDelivery;
    if (U->isPickup())
    {
        uPickup = U;
        uDelivery = U + 1;
    }
    else
    {
        uDelivery = U;
        uPickup = U - 1;
    }

    Route::Node *vPickup, *vDelivery;
    if (V->isPickup())
    {
        vPickup = V;
        vDelivery = V + 1;
    }
    else
    {
        vDelivery = V;
        vPickup = V - 1;
    }

    Route::swap(vPickup, uPickup);
    Route::swap(vDelivery, uDelivery);
}

template <>
bool pyvrp::search::supports<ReplaceOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
