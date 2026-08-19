#include "RelocatePickup.h"

#include "PickupSegment.h"

#include <cassert>

using pyvrp::search::RelocatePickup;

pyvrp::Cost RelocatePickup::evalAfter(Route::Node *U,
                                      CostEvaluator const &costEvaluator)
{
    assert(U->route() && U->isPickup());
    auto const *route = U->route();
    auto const *delivery = U + 1;

    auto between = route->between<true>(U->pos() + 1, U->pos() + 1);
    for (auto const *node = n(U); node != delivery; node = n(node), ++between)
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(U->pos() - 1),
                                                between,
                                                PickupSegment(data, U->idx()),
                                                route->after(node->pos() + 1)));

        if (deltaCost < 0)
        {
            move_.before = n(node);
            return deltaCost;
        }
    }

    return 0;
}

pyvrp::Cost RelocatePickup::evalBefore(Route::Node *U,
                                       CostEvaluator const &costEvaluator)
{
    assert(U->route() && U->isPickup());
    auto const *route = U->route();

    auto between = route->between<true>(U->pos() - 1, U->pos() - 1);
    for (auto const *node = p(U); !node->isDepot(); node = p(node), --between)
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(node->pos() - 1),
                                                PickupSegment(data, U->idx()),
                                                between,
                                                route->after(U->pos() + 1)));

        if (deltaCost < 0)
        {
            move_.before = node;
            return deltaCost;
        }
    }

    return 0;
}

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

    auto deltaCost = evalBefore(U, costEvaluator);
    if (deltaCost < 0)
        return std::make_pair(deltaCost, true);

    deltaCost = evalAfter(U, costEvaluator);
    return std::make_pair(deltaCost, deltaCost < 0);
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
