#include "RelocateDelivery.h"

#include "DeliverySegment.h"

#include <cassert>

using pyvrp::search::RelocateDelivery;

std::pair<pyvrp::Cost, bool>
RelocateDelivery::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    move_ = {};

    auto const *route = U->route();
    auto const *delivery = U + 1;
    assert(delivery->route() == route && delivery->pos() > U->pos());

    for (auto const *after = U; !after->isDepot(); after = n(after))
    {
        if (after == delivery || n(after) == delivery)
            continue;

        Cost deltaCost = 0;
        if (delivery->pos() < after->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(delivery->pos() - 1),
                    route->between(delivery->pos() + 1, after->pos()),
                    DeliverySegment(data, U->idx()),
                    route->after(after->pos() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(after->pos()),
                    DeliverySegment(data, U->idx()),
                    route->between(after->pos() + 1, delivery->pos() - 1),
                    route->after(delivery->pos() + 1)));

        if (deltaCost < 0)
        {
            move_.cost = deltaCost;
            move_.after = after;
            break;
        }
    }

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocateDelivery::apply(Route::Node *U) const
{
    assert(U->isPickup() && U->route() && move_.after);
    stats_.numApplications++;

    auto *route = U->route();
    auto *delivery = U + 1;
    route->remove(delivery->pos());
    route->insert(move_.after->pos() + 1, delivery);
}

std::string RelocateDelivery::name() const { return "RelocateDelivery"; }

bool RelocateDelivery::supports(ProblemData const &data)
{
    return data.numShipments() > 0;
}
