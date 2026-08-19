#include "RelocateDelivery.h"

#include "DeliverySegment.h"

#include <cassert>

using pyvrp::search::RelocateDelivery;

pyvrp::Cost RelocateDelivery::evalAfter(Route::Node *U,
                                        CostEvaluator const &costEvaluator)
{
    assert(U->route() && U->isPickup());
    auto const *route = U->route();
    auto const *deliv = U + 1;

    auto between = route->between<true>(deliv->pos() + 1, deliv->pos() + 1);
    for (auto const *node = n(deliv); !node->isDepot();
         node = n(node), ++between)
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(deliv->pos() - 1),
                                                between,
                                                DeliverySegment(data, U->idx()),
                                                route->after(node->pos() + 1)));

        if (deltaCost < 0)
        {
            move_.after = node;
            return deltaCost;
        }
    }

    return 0;
}

pyvrp::Cost RelocateDelivery::evalBefore(Route::Node *U,
                                         CostEvaluator const &costEvaluator)
{
    assert(U->route() && U->isPickup());
    auto const *route = U->route();
    auto const *deliv = U + 1;
    assert(deliv->pos() > U->pos());

    auto between = route->between<true>(deliv->pos() - 1, deliv->pos() - 1);
    for (auto const *node = p(deliv); node != U; node = p(node), --between)
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(route->before(node->pos() - 1),
                            DeliverySegment(data, U->idx()),
                            between,
                            route->after(deliv->pos() + 1)));

        if (deltaCost < 0)
        {
            move_.after = p(node);
            return deltaCost;
        }
    }

    return 0;
}

std::pair<pyvrp::Cost, bool>
RelocateDelivery::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    move_ = {};

    auto deltaCost = evalBefore(U, costEvaluator);
    if (deltaCost < 0)
        return std::make_pair(deltaCost, true);

    deltaCost = evalAfter(U, costEvaluator);
    return std::make_pair(deltaCost, deltaCost < 0);
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
