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

    // Don't tackle the case that U or V is a depot unload node, which could
    // result in inconsistent routes after swapping. Could end up with trips
    // which do not start and end at a depot load and unload node.
    if (U->isDepotUnload() || V->isDepotUnload())
        return 0;

    Cost deltaCost = 0;

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    if (uRoute->empty() && n(V)->idx() < vRoute->size() - 1)
        deltaCost += uRoute->fixedVehicleCost();

    if (vRoute->empty() && n(U)->idx() < uRoute->size() - 1)
        deltaCost += vRoute->fixedVehicleCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->idx() == 0 && n(V)->idx() == vRoute->size() - 1)
        deltaCost -= uRoute->fixedVehicleCost();

    if (!vRoute->empty() && V->idx() == 0 && n(U)->idx() == uRoute->size() - 1)
        deltaCost -= vRoute->fixedVehicleCost();

    // When multiple trips are involved, then the start and end depots of the
    // two routes must be the same.
    size_t tripsTailU = uRoute->numTrips() - U->tripIdx();
    size_t tripsTailV = vRoute->numTrips() - V->tripIdx();
    if (tripsTailU > 1 || tripsTailV > 1)
    {
        if (uRoute->startDepot() != vRoute->startDepot()
            || uRoute->endDepot() != vRoute->endDepot())
            return 0;
    }

    // Maximum number of trips must not be exceeded.
    size_t numTripsU = U->tripIdx() + tripsTailV;
    if (U->isDepotLoad() && n(V)->isDepotUnload())
        numTripsU--;  // Trip will become empty.

    if (numTripsU > uRoute->maxTrips())
        return 0;

    size_t numTripsV = V->tripIdx() + tripsTailU;
    if (V->isDepotLoad() && n(U)->isDepotUnload())
        numTripsV--;  // Trip will become empty.

    if (numTripsV > vRoute->maxTrips())
        return 0;

    // Note that a proposal can contain an empty trip, but this should not
    // affect delta cost calculation.
    size_t const vLastClientIndex = vRoute->size() - 2;
    size_t const uLastClientIndex = uRoute->size() - 2;
    if (U->idx() < uLastClientIndex && V->idx() < vLastClientIndex)
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
    else if (U->idx() < uLastClientIndex && V->idx() >= vLastClientIndex)
    {
        // Route U "sends" clients to route V, but not the other way around.
        auto const uProposal = uRoute->proposal(uRoute->before(U->idx()),
                                                uRoute->at(uRoute->size() - 1));

        auto const vProposal = vRoute->proposal(
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size() - 2),
            vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (U->idx() >= uLastClientIndex && V->idx() < vLastClientIndex)
    {
        // Route V "sends" clients to route U, but not the other way around.
        auto const uProposal = uRoute->proposal(
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size() - 2),
            uRoute->at(uRoute->size() - 1));

        auto const vProposal = vRoute->proposal(vRoute->before(V->idx()),
                                                vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }

    return deltaCost;
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    // First swap the first trip of route U with the first trip of route V.
    // Note that this can result in an empty trip in one of the routes, when
    // one of the routes will swap all the clients of the trip and the other
    // route will swap no clients of the trip. We will tackle this at the end.
    auto *moveVAfter = n(V);
    auto *moveUAfter = n(U);

    auto *insertUAfter = U;
    auto insertIdxU = insertUAfter->idx() + 1;
    while (!moveVAfter->isDepot())
    {
        auto *node = moveVAfter;
        moveVAfter = n(moveVAfter);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdxU++, node);
    }

    auto *insertVAfter = V;
    auto insertIdxV = insertVAfter->idx() + 1;
    while (!moveUAfter->isDepot())
    {
        auto *node = moveUAfter;
        moveUAfter = n(moveUAfter);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdxV++, node);
    }

    insertUAfter = moveUAfter;
    insertVAfter = moveVAfter;

    // Move rest of the trips 1 by 1. Note that the max trips in a route can
    // temporarily be exceeded by 1, but this will be corrected.
    while (moveVAfter->idx() < V->route()->size() - 1
           || moveUAfter->idx() < U->route()->size() - 1)
    {
        // Move trip from route V to route U.
        insertIdxU = insertUAfter->idx() + 1;
        if (moveVAfter->idx() < V->route()->size() - 1)
        {
            auto *prevVAfter = moveVAfter;
            moveVAfter = n(moveVAfter);
            assert(moveVAfter->isDepotLoad());
            U->route()->insertTrip(insertUAfter->tripIdx() + 1);
            insertIdxU++;
            moveUAfter = n(n(moveUAfter));
            while (!moveVAfter->isDepotUnload())
            {
                moveVAfter = n(moveVAfter);
                auto *node = moveVAfter;
                assert(!node->isDepotLoad());
                if (node->isDepotUnload())
                {
                    insertUAfter = moveUAfter;
                    V->route()->removeTrip(moveVAfter->tripIdx());
                    moveVAfter = prevVAfter;
                }
                else  // Client
                {
                    assert(node->isClient());
                    moveVAfter = p(moveVAfter);
                    V->route()->remove(node->idx());
                    U->route()->insert(insertIdxU++, node);
                }
            }
        }

        // Move trip from route U to route V.
        insertIdxV = insertVAfter->idx() + 1;
        if (moveUAfter->idx() < U->route()->size() - 1)
        {
            auto *prevUAfter = moveUAfter;
            moveUAfter = n(moveUAfter);
            assert(moveUAfter->isDepotLoad());
            V->route()->insertTrip(insertVAfter->tripIdx() + 1);
            insertIdxV++;
            moveVAfter = n(n(moveVAfter));
            while (!moveUAfter->isDepotUnload())
            {
                moveUAfter = n(moveUAfter);
                auto *node = moveUAfter;
                assert(!node->isDepotLoad());
                if (node->isDepotUnload())
                {
                    insertVAfter = moveVAfter;
                    U->route()->removeTrip(moveUAfter->tripIdx());
                    moveUAfter = prevUAfter;
                }
                else  // Client
                {
                    assert(node->isClient());
                    moveUAfter = p(moveUAfter);
                    U->route()->remove(node->idx());
                    V->route()->insert(insertIdxV++, node);
                }
            }
        }
    }

    // Remove empty trips as result of first trip swap.
    if (U->route()->numTrips() > 1 && U->isDepotLoad() && n(U)->isDepotUnload())
        U->route()->removeTrip(U->tripIdx());

    if (V->route()->numTrips() > 1 && V->isDepotLoad() && n(V)->isDepotUnload())
        V->route()->removeTrip(V->tripIdx());
}
