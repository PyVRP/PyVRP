#include "RelocatePickup.h"

#include "PickupSegment.h"

#include <cassert>

using pyvrp::search::RelocatePickup;

std::pair<pyvrp::Cost, bool>
RelocatePickup::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    move_ = {};

    auto const *route = U->route();
    auto const *delivery = U + 1;
    assert(delivery->route() == route && delivery->pos() > U->pos());

    // Evaluate reinserting U just before node. We store and apply the
    // best-found move.
    for (auto const *node = delivery; !node->isDepot(); node = p(node))
    {
        if (node == U || p(node) == U)
            continue;

        Cost deltaCost = 0;
        if (U->pos() < node->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(U->pos() - 1),
                                route->between(U->pos() + 1, node->pos() - 1),
                                PickupSegment(data, U->idx()),
                                route->after(node->pos())));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(node->pos() - 1),
                                PickupSegment(data, U->idx()),
                                route->between(node->pos(), U->pos() - 1),
                                route->after(U->pos() + 1)));

        if (deltaCost < move_.cost)
        {
            move_.cost = deltaCost;
            move_.before = node;
        }
    }

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocatePickup::apply(Route::Node *U) const
{
    assert(U->isPickup() && U->route() && move_.before);
    stats_.numApplications++;

    auto *route = U->route();
    route->remove(U->pos());
    route->insert(move_.before->pos(), U);
}

std::string RelocatePickup::name() const { return "RelocatePickup"; }

bool RelocatePickup::supports(ProblemData const &data)
{
    return data.numShipments() > 0;
}
