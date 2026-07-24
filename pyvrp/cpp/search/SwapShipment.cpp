#include "SwapShipment.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <cassert>

using pyvrp::search::SwapShipment;

std::pair<pyvrp::Cost, bool> SwapShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(V->route());
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route() || !V->isShipment())
        return std::make_pair(0, false);

    if (U->route() == V->route())
        // It is not technically impossible to swap within the same route, but
        // evaluating every possible route configuration requires dozens of
        // branches and proposals, which is prohibitive to fully list here.
        return std::make_pair(0, false);

    Route::Node *uPickup = U;
    Route::Node *uDelivery = U + 1;
    Route::Node *vPickup, *vDelivery;
    if (V->isPickup())
    {
        vPickup = V;
        vDelivery = V + 1;
    }
    else
    {
        vPickup = V - 1;
        vDelivery = V;
    }

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    // Four cases, depending on whether there are nodes between the PD pairs.
    Cost deltaCost = 0;
    if (n(uPickup) == uDelivery && n(vPickup) == vDelivery)
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(uPickup->pos() - 1),
                              vRoute->between(vPickup->pos(), vDelivery->pos()),
                              uRoute->after(uDelivery->pos() + 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(vPickup->pos() - 1),
                              uRoute->between(uPickup->pos(), uDelivery->pos()),
                              vRoute->after(vDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (n(uPickup) == uDelivery)
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(uPickup->pos() - 1),
                              vRoute->at(vPickup->pos()),
                              vRoute->at(vDelivery->pos()),
                              uRoute->after(uDelivery->pos() + 1));

        auto const vProposal = Route::Proposal(
            vRoute->before(vPickup->pos() - 1),
            uRoute->at(uPickup->pos()),
            vRoute->between(vPickup->pos() + 1, vDelivery->pos() - 1),
            uRoute->at(uDelivery->pos()),
            vRoute->after(vDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (n(vPickup) == vDelivery)
    {
        auto const uProposal = Route::Proposal(
            uRoute->before(uPickup->pos() - 1),
            vRoute->at(vPickup->pos()),
            uRoute->between(uPickup->pos() + 1, uDelivery->pos() - 1),
            vRoute->at(vDelivery->pos()),
            uRoute->after(uDelivery->pos() + 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(vPickup->pos() - 1),
                              uRoute->at(uPickup->pos()),
                              uRoute->at(uDelivery->pos()),
                              vRoute->after(vDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else
    {
        auto const uProposal = Route::Proposal(
            uRoute->before(uPickup->pos() - 1),
            vRoute->at(vPickup->pos()),
            uRoute->between(uPickup->pos() + 1, uDelivery->pos() - 1),
            vRoute->at(vDelivery->pos()),
            uRoute->after(uDelivery->pos() + 1));

        auto const vProposal = Route::Proposal(
            vRoute->before(vPickup->pos() - 1),
            uRoute->at(uPickup->pos()),
            vRoute->between(vPickup->pos() + 1, vDelivery->pos() - 1),
            uRoute->at(uDelivery->pos()),
            vRoute->after(vDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }

    return std::make_pair(deltaCost, deltaCost < 0);
}

void SwapShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isPickup() && U->route() && V->isShipment() && V->route());
    stats_.numApplications++;

    if (V->isPickup())
    {
        Route::swap(U, V);
        Route::swap(U + 1, V + 1);
    }
    else
    {
        assert(V->isDelivery());
        Route::swap(U, V - 1);
        Route::swap(U + 1, V);
    }
}

std::string SwapShipment::name() const { return "SwapShipment"; }

bool SwapShipment::supports(ProblemData const &data)
{
    // Needs multiple shipments and does not work for a TSP.
    return data.numVehicles() > 1 && data.numShipments() > 1;
}
