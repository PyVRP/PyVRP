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
    auto const &route = *V->route();

    if (U->isPickup())  // pickup after V, delivery later in the route
    {
        if (route.empty())  // then V is the start depot, and we want to insert
        {                   // U's pickup and delivery directly after
            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route.before(V->pos()),
                                PickupSegment(data, U->idx()),
                                DeliverySegment(data, U->idx()),
                                route.after(V->pos() + 1)));

            if (deltaCost < 0)
                move_ = {V->pos() + 1};

            return std::make_pair(deltaCost, deltaCost < 0);
        }

        for (size_t pos = V->pos() + 1; pos != route.size() - 1; ++pos)
        {
            if (route[pos]->isDepot())  // shipments cannot cross depots
                break;

            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route.before(V->pos()),
                                PickupSegment(data, U->idx()),
                                route.between(V->pos() + 1, pos),
                                DeliverySegment(data, U->idx()),
                                route.after(pos + 1)));

            if (deltaCost < 0)
            {
                move_ = {pos};
                return std::make_pair(deltaCost, true);
            }
        }
    }
    else if (!V->isStartDepot())  // delivery after V, pickup earlier in route
        for (size_t pos = V->pos() - 1; pos != 0; --pos)
        {
            if (route[pos]->isDepot())  // shipments cannot cross depots
                break;

            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route.before(pos - 1),
                                PickupSegment(data, U->idx()),
                                route.between(pos, V->pos()),
                                DeliverySegment(data, U->idx()),
                                route.after(V->pos() + 1)));

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
    stats_.numApplications++;

    auto *route = V->route();

    if (U->isPickup())
    {
        assert(move_.pos > V->pos());  // delivery at pos, after pickup
        route->insert(move_.pos, U + 1);
        route->insert(V->pos() + 1, U);
    }
    else
    {
        assert(move_.pos < V->pos());  // pickup at pos, before delivery
        route->insert(V->pos() + 1, U);
        route->insert(move_.pos, U - 1);
    }
}

std::string InsertOptionalShipment::name() const
{
    return "InsertOptionalShipment";
}

template <>
bool pyvrp::search::supports<InsertOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
