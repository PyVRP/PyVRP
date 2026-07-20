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

    if (!U->isPickup() || U->route() || !V->route())
        return std::make_pair(0, false);

    move_ = {};

    auto const &shipment = data.shipment(U->idx());
    auto const &route = *V->route();

    Cost const fixedCost = route.empty() ? route.fixedVehicleCost() : 0;

    Cost deltaCost = fixedCost - shipment.prize;
    costEvaluator.deltaCost(deltaCost,  // delivery directly after pickup
                            Route::Proposal(route.before(V->pos()),
                                            PickupSegment(data, U->idx()),
                                            DeliverySegment(data, U->idx()),
                                            route.after(V->pos() + 1)));

    if (deltaCost < 0)
    {
        move_ = {V->pos() + 1};
        return std::make_pair(deltaCost, true);
    }

    // Pickup after V, delivery later in the route.
    for (auto const *node = n(V); !node->isDepot(); node = n(node))
    {
        Cost deltaCost = fixedCost - shipment.prize;
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(route.before(V->pos()),
                            PickupSegment(data, U->idx()),
                            route.between(V->pos() + 1, node->pos()),
                            DeliverySegment(data, U->idx()),
                            route.after(node->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {node->pos() + 1};  // after node
            return std::make_pair(deltaCost, true);
        }
    }

    return std::make_pair(0, false);
}

void InsertOptionalShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isPickup() && !U->route() && V->route());
    stats_.numApplications++;

    auto *route = V->route();
    assert(move_.pos > V->pos());  // delivery at pos, after pickup
    route->insert(move_.pos, U + 1);
    route->insert(V->pos() + 1, U);
}

std::string InsertOptionalShipment::name() const
{
    return "InsertOptionalShipment";
}

bool InsertOptionalShipment::supports(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
