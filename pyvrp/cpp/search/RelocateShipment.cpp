#include "RelocateShipment.h"

#include <cassert>

using pyvrp::search::RelocateShipment;

std::pair<pyvrp::Cost, bool> RelocateShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(V->route());
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    if (U->route() == V->route())
        // TODO
        return std::make_pair(0, false);

    move_ = {};

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    auto const *uPickup = U;
    auto const *uDelivery = U + 1;

    Cost removeCost = 0;
    if (uRoute->numShipments() == 1 && uRoute->numClients() == 0)
        removeCost -= uRoute->fixedVehicleCost();

    if (n(uPickup) != uDelivery)
    {
        auto const uProposal = Route::Proposal(
            uRoute->before(uPickup->pos() - 1),
            uRoute->between(uPickup->pos() + 1, uDelivery->pos() - 1),
            uRoute->after(uDelivery->pos() + 1));

        costEvaluator.deltaCost<true>(removeCost, uProposal);
    }
    else
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(uPickup->pos() - 1),
                              uRoute->after(uDelivery->pos() + 1));

        costEvaluator.deltaCost<true>(removeCost, uProposal);
    }

    Cost deltaCost = removeCost;
    costEvaluator.deltaCost(deltaCost,  // delivery directly after pickup
                            Route::Proposal(vRoute->before(V->pos()),
                                            uRoute->at(uPickup->pos()),
                                            uRoute->at(uDelivery->pos()),
                                            vRoute->after(V->pos() + 1)));

    if (deltaCost < 0)
    {
        move_ = {V->pos() + 1};
        return std::make_pair(deltaCost, true);
    }

    // Pickup after V, delivery later in the route.
    for (auto const *node = n(V); !node->isDepot(); node = n(node))
    {
        Cost deltaCost = removeCost;
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(vRoute->before(V->pos()),
                            uRoute->at(uPickup->pos()),
                            vRoute->between(V->pos() + 1, node->pos()),
                            uRoute->at(uDelivery->pos()),
                            vRoute->after(node->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {node->pos() + 1};  // after node
            return std::make_pair(deltaCost, true);
        }
    }

    return std::make_pair(0, false);
}

void RelocateShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isPickup() && U->route() && V->route());
    stats_.numApplications++;

    auto *uRoute = U->route();
    uRoute->remove((U + 1)->pos());  // remove delivery
    uRoute->remove(U->pos());        // remove pickup

    auto *vRoute = V->route();
    assert(move_.pos > V->pos());
    vRoute->insert(move_.pos, U + 1);  // insert delivery
    vRoute->insert(V->pos() + 1, U);   // insert pickup
}

std::string RelocateShipment::name() const { return "RelocateShipment"; }

bool RelocateShipment::supports(ProblemData const &data)
{
    return data.numShipments() > 1;  // needs multiple shipments
}
