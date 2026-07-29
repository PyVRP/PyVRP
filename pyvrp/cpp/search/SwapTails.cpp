#include "SwapTails.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::SwapTails;

namespace
{
bool onLastTrip(pyvrp::search::Route::Node *node)
{
    auto const *route = node->route();
    return node->trip() + 1 == route->numTrips();
}

bool splitsShipment(pyvrp::search::Route::Node *node)
{
    auto const *route = node->route();
    return route->numPickups(node->pos()) != route->numDeliveries(node->pos());
}
}  // namespace

std::pair<pyvrp::Cost, bool> SwapTails::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;
    assert(!U->isDepot());
    assert(!V->isEndDepot() && !V->isReloadDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (!uRoute || uRoute == vRoute)
        return std::make_pair(0, false);  // unassigned, or same route

    if (uRoute > vRoute && !uRoute->empty() && !vRoute->empty())
        return std::make_pair(0, false);  // move will be tackled later

    if (!onLastTrip(U) || !onLastTrip(V))
        // We cannot move reload depots, so we only evaluate a move if it does
        // not include a reload depot.
        return std::make_pair(0, false);

    if (splitsShipment(U) || splitsShipment(V))
        // Cannot evaluate this move because it would leave part of a shipment
        // in the other route.
        return std::make_pair(0, false);

    Cost deltaCost = 0;

    if (!n(U)->isEndDepot() && !n(V)->isEndDepot())
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(U->pos()),
                              vRoute->between(V->pos() + 1, vRoute->size() - 2),
                              uRoute->at(uRoute->size() - 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(V->pos()),
                              uRoute->between(U->pos() + 1, uRoute->size() - 2),
                              vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (!n(U)->isEndDepot() && n(V)->isEndDepot())
    {
        auto const uProposal = Route::Proposal(uRoute->before(U->pos()),
                                               uRoute->at(uRoute->size() - 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(V->pos()),
                              uRoute->between(U->pos() + 1, uRoute->size() - 2),
                              vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (n(U)->isEndDepot() && !n(V)->isEndDepot())
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(U->pos()),
                              vRoute->between(V->pos() + 1, vRoute->size() - 2),
                              uRoute->at(uRoute->size() - 1));

        if (V->isStartDepot())
        {
            deltaCost -= costEvaluator.penalisedCost(*vRoute);
            costEvaluator.deltaCost(deltaCost, uProposal);
        }
        else
        {
            auto const vProposal = Route::Proposal(
                vRoute->before(V->pos()), vRoute->at(vRoute->size() - 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
    }

    return std::make_pair(deltaCost, deltaCost < 0);
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertIdx = U->pos() + 1;
    while (!nV->isEndDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->pos());
        U->route()->insert(insertIdx++, node);
    }

    insertIdx = V->pos() + 1;
    while (!nU->isEndDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->pos());
        V->route()->insert(insertIdx++, node);
    }
}

std::string SwapTails::name() const { return "SwapTails"; }

bool SwapTails::supports(ProblemData const &data)
{
    // Does not work for TSP, since the operator needs two routes. Also requires
    // at least one client, as swapping whole tails does not make much sense
    // for pure shipments since it likely breaks pickup-and-delivery pairs.
    return data.numVehicles() > 1 && data.numClients() > 0;
}
