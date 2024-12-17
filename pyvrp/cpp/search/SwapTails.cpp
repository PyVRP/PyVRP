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
    if (U->type() == Route::Node::NodeType::DepotUnload
        || V->type() == Route::Node::NodeType::DepotUnload)
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
    if (U->type() == Route::Node::NodeType::DepotLoad
        && n(V)->type() == Route::Node::NodeType::DepotUnload)
        numTripsU--;  // Trip will become empty.

    if (numTripsU > uRoute->maxTrips())
        return 0;

    size_t numTripsV = V->tripIdx() + tripsTailU;
    if (V->type() == Route::Node::NodeType::DepotLoad
        && n(U)->type() == Route::Node::NodeType::DepotUnload)
        numTripsV--;  // Trip will become empty.

    if (numTripsV > vRoute->maxTrips())
        return 0;

    // Note that a proposal can contain an empty trip, but this should not
    // affect delta cost calculation.
    if (U->idx() < uRoute->size() - 2 && V->idx() < vRoute->size() - 2)
    {
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
    else if (U->idx() < uRoute->size() - 2 && V->idx() >= vRoute->size() - 2)
    {
        auto const uProposal = uRoute->proposal(uRoute->before(U->idx()),
                                                uRoute->at(uRoute->size() - 1));

        auto const vProposal = vRoute->proposal(
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size() - 2),
            vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (U->idx() >= uRoute->size() - 2 && V->idx() < vRoute->size() - 2)
    {
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
            assert(moveVAfter->type() == Route::Node::NodeType::DepotLoad);
            U->route()->insertTrip(insertIdxU++);
            moveUAfter = n(n(moveUAfter));
            while (moveVAfter->type() != Route::Node::NodeType::DepotUnload)
            {
                moveVAfter = n(moveVAfter);
                auto *node = moveVAfter;
                assert(node->type() != Route::Node::NodeType::DepotLoad);
                if (node->type() == Route::Node::NodeType::DepotUnload)
                {
                    insertUAfter = moveUAfter;
                    V->route()->removeTrip(moveVAfter->tripIdx());
                    moveVAfter = prevVAfter;
                }
                else  // Client
                {
                    assert(!node->isDepot());
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
            assert(moveUAfter->type() == Route::Node::NodeType::DepotLoad);
            V->route()->insertTrip(insertIdxV++);
            moveVAfter = n(n(moveVAfter));
            while (moveUAfter->type() != Route::Node::NodeType::DepotUnload)
            {
                moveUAfter = n(moveUAfter);
                auto *node = moveUAfter;
                assert(node->type() != Route::Node::NodeType::DepotLoad);
                if (node->type() == Route::Node::NodeType::DepotUnload)
                {
                    insertVAfter = moveVAfter;
                    U->route()->removeTrip(moveUAfter->tripIdx());
                    moveUAfter = prevUAfter;
                }
                else  // Client
                {
                    assert(!node->isDepot());
                    moveUAfter = p(moveUAfter);
                    U->route()->remove(node->idx());
                    V->route()->insert(insertIdxV++, node);
                }
            }
        }
    }

    // Remove empty trips as result of first trip swap.
    if (U->route()->numTrips() > 1
        && U->type() == Route::Node::NodeType::DepotLoad
        && n(U)->type() == Route::Node::NodeType::DepotUnload)
        U->route()->removeTrip(U->tripIdx());

    if (V->route()->numTrips() > 1
        && V->type() == Route::Node::NodeType::DepotLoad
        && n(V)->type() == Route::Node::NodeType::DepotUnload)
        V->route()->removeTrip(V->tripIdx());
}
