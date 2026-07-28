#include "RemoveOptionalShipment.h"

#include <cassert>

using pyvrp::search::RemoveOptionalShipment;

std::pair<pyvrp::Cost, bool>
RemoveOptionalShipment::evaluate(Route::Node *U,
                                 CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || !U->isPickup())
        return std::make_pair(0, false);

    auto const &shipment = data.shipment(U->idx());
    if (shipment.required)
        return std::make_pair(0, false);

    auto const *pickup = U;
    auto const *delivery = U + 1;
    auto const *route = U->route();

    Cost deltaCost = shipment.prize;
    if (route->numShipments() == 1 && route->numClients() == 0)
    {
        deltaCost -= costEvaluator.penalisedCost(*route);
        return std::make_pair(deltaCost, deltaCost < 0);
    }

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
    assert(U->isPickup() && U->route());
    stats_.numApplications++;

    auto *route = U->route();
    route->remove((U + 1)->pos());  // U + 1 is delivery
    route->remove(U->pos());        // U is pickup
}

std::string RemoveOptionalShipment::name() const
{
    return "RemoveOptionalShipment";
}

bool RemoveOptionalShipment::supports(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
