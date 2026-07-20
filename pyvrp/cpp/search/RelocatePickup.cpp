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

    auto *route = U->route();
    auto *delivery = U + 1;
    assert(delivery->route() == route && delivery->pos() > U->pos());

    for (auto const *node = p(delivery); !node->isDepot(); node = p(node))
    {
        if (node == U || n(node) == U)
            continue;

        Cost deltaCost = 0;
        if (U->pos() < node->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(U->pos() - 1),
                                route->between(U->pos() + 1, node->pos()),
                                PickupSegment(data, U->idx()),
                                route->after(node->pos() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(node->pos()),
                                PickupSegment(data, U->idx()),
                                route->between(node->pos() + 1, U->pos() - 1),
                                route->after(U->pos() + 1)));

        if (deltaCost < move_.cost)
        {
            move_.cost = deltaCost;
            move_.after = node;
        }
    }

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocatePickup::apply(Route::Node *U) const
{
    assert(U->isPickup() && U->route() && move_.after);
    stats_.numApplications++;

    auto *route = U->route();
    route->remove(U->pos());
    route->insert(move_.after->pos() + 1, U);
}

std::string RelocatePickup::name() const { return "RelocatePickup"; }

bool RelocatePickup::supports(ProblemData const &data)
{
    return data.numShipments() > 0;
}
