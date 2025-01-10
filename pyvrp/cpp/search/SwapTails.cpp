#include "SwapTails.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::SwapTails;

pyvrp::Cost SwapTails::evaluate(Route::Node *U,
                                Route::Node *V,
                                CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (uRoute == vRoute || uRoute->idx() > vRoute->idx())
        return 0;  // same route, or move will be tackled in a later iteration

    // Don't tackle the case that either U or V is an end depot node, which
    // would result in inconsistent routes after swapping. It would lead to
    // trips which do not begin and finish at a start and end depot node.
    if ((U->isEndDepot() && !V->isEndDepot())
        || (!U->isEndDepot() && V->isEndDepot()))
        return 0;

    Cost deltaCost = 0;

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    bool const vTailEmpty = V->idx() >= vRoute->size() - 2;
    bool const uTailEmpty = U->idx() >= uRoute->size() - 2;
    if (uRoute->empty() && !vTailEmpty)
        deltaCost += uRoute->fixedVehicleCost();

    if (vRoute->empty() && !uTailEmpty)
        deltaCost += vRoute->fixedVehicleCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->idx() == 0 && vTailEmpty)
        deltaCost -= uRoute->fixedVehicleCost();

    if (!vRoute->empty() && V->idx() == 0 && uTailEmpty)
        deltaCost -= vRoute->fixedVehicleCost();

    // When multiple trips are involved, then the start and end depots of the
    // two routes must be the same.
    size_t numTripsTailU = uRoute->numTrips() - U->tripIdx();
    size_t numTripsTailV = vRoute->numTrips() - V->tripIdx();
    if ((numTripsTailU > 1 || numTripsTailV > 1)
        && (uRoute->startDepot() != vRoute->startDepot()
            || uRoute->endDepot() != vRoute->endDepot()))
        return 0;

    // Maximum number of trips must not be exceeded.
    size_t numTripsU = U->tripIdx() + numTripsTailV;
    if (U->isStartDepot() && n(V)->isEndDepot())
        numTripsU--;  // Trip will become empty.

    if (numTripsU > uRoute->maxTrips())
        return 0;

    size_t numTripsV = V->tripIdx() + numTripsTailU;
    if (V->isStartDepot() && n(U)->isEndDepot())
        numTripsV--;  // Trip will become empty.

    if (numTripsV > vRoute->maxTrips())
        return 0;

    // Note that a proposal can contain an empty trip, but this should not
    // affect delta cost calculation. However, this is only true given that
    // there is no loading duration at the depot for the start of a trip.
    if (!uTailEmpty && !vTailEmpty)
    {
        // Both routes "send and receive" clients to/from each other.
        auto const uProposal = uRoute->proposal(
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size() - 2),
            uRoute->at(uRoute->size() - 1));

        auto const vProposal = vRoute->proposal(
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size() - 2),
            vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (!uTailEmpty && vTailEmpty)
    {
        // Route U "sends" clients to route V, but not the other way around.
        auto const vProposal = vRoute->proposal(
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size() - 2),
            vRoute->at(vRoute->size() - 1));

        if (U->isEndDepot())
        {
            // Proposal with one segment is currently not supported.
            auto const uProposal = uRoute->proposal(
                uRoute->before(U->idx() - 1), uRoute->at(uRoute->size() - 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
        else
        {
            auto const uProposal = uRoute->proposal(
                uRoute->before(U->idx()), uRoute->at(uRoute->size() - 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
    }
    else if (uTailEmpty && !vTailEmpty)
    {
        // Route V "sends" clients to route U, but not the other way around.
        auto const uProposal = uRoute->proposal(
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size() - 2),
            uRoute->at(uRoute->size() - 1));

        if (V->isEndDepot())
        {
            // Proposal with one segment is currently not supported.
            auto const vProposal = vRoute->proposal(
                vRoute->before(V->idx() - 1), vRoute->at(vRoute->size() - 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
        else
        {
            auto const vProposal = vRoute->proposal(
                vRoute->before(V->idx()), vRoute->at(vRoute->size() - 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
    }

    return deltaCost;
}

std::pair<size_t, size_t> SwapTails::applySwapTripTails(Route::Node *U,
                                                        Route::Node *V) const
{
    // This method should not be used for swapping entire trips.
    assert(!U->isEndDepot() && !V->isEndDepot());

    auto *nV = n(V);
    auto *nU = n(U);

    // Move trip tail from route V to route U.
    size_t insertIdxU = U->idx() + 1;
    while (!nV->isEndDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdxU++, node);
    }

    // Move trip tail from route U to route V.
    size_t insertIdxV = V->idx() + 1;
    while (!nU->isEndDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdxV++, node);
    }

    return {nU->idx(), nV->idx()};
}

std::pair<size_t, size_t> SwapTails::applySwapTrips(Route::Node *U,
                                                    Route::Node *V) const
{
    assert(U->isEndDepot() && V->isEndDepot());

    // Skip start depot of next trip if next trip exists.
    auto *nU = (U->tripIdx() + 1 < U->route()->numTrips()) ? n(n(U)) : U;
    auto *nV = (V->tripIdx() + 1 < V->route()->numTrips()) ? n(n(V)) : V;

    // Move trip (if any) from route V to route U.
    size_t insertIdxU = U->idx();
    if (nV != V)
    {
        U->route()->insertTrip(U->tripIdx() + 1);
        insertIdxU += 2;  // Insert after the new start depot.
        while (!nV->isEndDepot())
        {
            auto *node = nV;
            nV = n(nV);
            V->route()->remove(node->idx());
            U->route()->insert(insertIdxU++, node);
        }

        // Remove empty trip from route V.
        V->route()->removeTrip(nV->tripIdx());
    }

    // Move trip (if any) from route U to route V.
    size_t insertIdxV = V->idx();
    if (nU != U)
    {
        V->route()->insertTrip(V->tripIdx() + 1);
        insertIdxV += 2;  // Insert after the new start depot.
        while (!nU->isEndDepot())
        {
            auto *node = nU;
            nU = n(nU);
            U->route()->remove(node->idx());
            V->route()->insert(insertIdxV++, node);
        }

        // Remove empty trip from route U.
        U->route()->removeTrip(nU->tripIdx());
    }

    return {insertIdxU, insertIdxV};
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    auto *curU = U;
    auto *curV = V;
    auto *uRoute = U->route();
    auto *vRoute = V->route();

    if (!U->isEndDepot() && !V->isEndDepot())
    {
        auto [idxU, idxV] = applySwapTripTails(curU, curV);
        curU = (*uRoute)[idxU];
        curV = (*vRoute)[idxV];
    }

    assert(curU->isEndDepot() && curV->isEndDepot());

    // Move rest of the trips 1 by 1. Note that the max trips in a route can
    // temporarily be exceeded by 1, but this will be corrected.
    while (curU->idx() < uRoute->size() - 1 || curV->idx() < vRoute->size() - 1)
    {
        auto [idxU, idxV] = applySwapTrips(curU, curV);
        curU = (*uRoute)[idxU];
        curV = (*vRoute)[idxV];
    }

    // Remove possible empty trip as result of first trip tail swap.
    if (!U->isEndDepot() && !V->isEndDepot())
    {
        if (uRoute->numTrips() > 1 && U->isStartDepot() && n(U)->isEndDepot())
            uRoute->removeTrip(U->tripIdx());

        if (vRoute->numTrips() > 1 && V->isStartDepot() && n(V)->isEndDepot())
            vRoute->removeTrip(V->tripIdx());
    }
}
