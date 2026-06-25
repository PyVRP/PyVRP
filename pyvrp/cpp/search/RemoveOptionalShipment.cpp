#include "RemoveOptionalShipment.h"

#include <cassert>

using pyvrp::search::RemoveOptionalShipment;

std::pair<pyvrp::Cost, bool>
RemoveOptionalShipment::evaluate(Route::Node *U,
                                 CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || !U->isShipment())
        return std::make_pair(0, false);

    auto const &shipment = data.shipment(U->idx());
    if (shipment.required)
        return std::make_pair(0, false);

    Route::Node *pickup, *delivery;
    if (U->isPickup())
    {
        pickup = U;
        delivery = U + 1;
    }
    else
    {
        delivery = U;
        pickup = U - 1;
    }

    assert(pickup->pos() < delivery->pos());
    auto const *route = U->route();

    Cost deltaCost = shipment.prize;
    if (n(pickup) == delivery)
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(route->before(pickup->pos() - 1),
                            route->after(delivery->pos() + 1)));
    else
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(
                route->before(pickup->pos() - 1),
                route->between(pickup->pos() + 1, delivery->pos() - 1),
                route->after(delivery->pos() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void RemoveOptionalShipment::apply(Route::Node *U) const
{
    assert(U->isShipment());
    stats_.numApplications++;

    Route::Node *pickup, *delivery;
    if (U->isPickup())
    {
        pickup = U;
        delivery = U + 1;
    }
    else
    {
        delivery = U;
        pickup = U - 1;
    }

    auto *route = U->route();
    route->remove(delivery->pos());
    route->remove(pickup->pos());
}

std::string RemoveOptionalShipment::name() const
{
    return "RemoveOptionalShipment";
}

template <>
bool pyvrp::search::supports<RemoveOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
