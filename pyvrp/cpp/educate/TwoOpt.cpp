#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

cost_type TwoOpt::evalWithinRoute(Node *U,
                                  Node *V,
                                  CostEvaluator const &costEvaluator) const
{
    if (U->position + 1 >= V->position)
        return 0;

    auto const *route = U->route;
    auto const currentCost = route->penalisedCost(costEvaluator);

    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();

    cost_type deltaDist
        = distMat(U->client, V->client) + distMat(n(U)->client, n(V)->client)
          + V->cumDeltaRevDist - distMat(U->client, n(U)->client)
          - distMat(V->client, n(V)->client) - n(U)->cumDeltaRevDist;

    // First compute bound based on dist and load
    auto const dist = route->dist() + deltaDist;
    auto const lbCost = costEvaluator.penalisedRouteCost(
        dist, route->load(), 0, data.vehicleCapacity());
    if (lbCost >= currentCost)
        return 0;

    // Compute time warp for route to get actual cost
    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(durMat, tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(durMat, tws, n(V)->twAfter);

    auto const cost = costEvaluator.penalisedRouteCost(
        dist, route->load(), tws.totalTimeWarp(), data.vehicleCapacity());
    return cost - currentCost;
}

cost_type TwoOpt::evalBetweenRoutes(Node *U,
                                    Node *V,
                                    CostEvaluator const &costEvaluator) const
{
    auto const currentCost = U->route->penalisedCost(costEvaluator)
                             + V->route->penalisedCost(costEvaluator);

    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();

    // Compute lower bound for new cost based on distance and load
    auto const distU = U->cumDist + distMat(U->client, n(V)->client)
                       + V->route->dist() - n(V)->cumDist;
    auto const distV = V->cumDist + distMat(V->client, n(U)->client)
                       + U->route->dist() - n(U)->cumDist;

    int const deltaLoad = U->cumLoad - V->cumLoad;
    auto const loadU = U->route->load() - deltaLoad;
    auto const loadV = V->route->load() + deltaLoad;

    auto const lbCostU = costEvaluator.penalisedRouteCost(
        distU, loadU, 0, data.vehicleCapacity());
    auto const lbCostV = costEvaluator.penalisedRouteCost(
        distV, loadV, 0, data.vehicleCapacity());

    if (lbCostU + lbCostV >= currentCost)
        return 0;

    // Add time warp for route U to get actual cost
    auto const uTWS = TWS::merge(durMat, U->twBefore, n(V)->twAfter);
    auto const costU = costEvaluator.penalisedRouteCost(
        distU, loadU, uTWS.totalTimeWarp(), data.vehicleCapacity());

    if (costU + lbCostV >= currentCost)
        return 0;

    // Add time warp for route V to get actual cost
    auto const vTWS = TWS::merge(durMat, V->twBefore, n(U)->twAfter);
    auto const costV = costEvaluator.penalisedRouteCost(
        distV, loadV, vTWS.totalTimeWarp(), data.vehicleCapacity());

    return costU + costV - currentCost;
}

void TwoOpt::applyWithinRoute(Node *U, Node *V) const
{
    auto *itRoute = V;
    auto *insertionPoint = U;
    auto *currNext = n(U);

    while (itRoute != currNext)  // No need to move x, we pivot around it
    {
        auto *current = itRoute;
        itRoute = p(itRoute);
        current->insertAfter(insertionPoint);
        insertionPoint = current;
    }
}

void TwoOpt::applyBetweenRoutes(Node *U, Node *V) const
{
    auto *itRouteU = n(U);
    auto *itRouteV = n(V);

    auto *insertLocation = U;
    while (!itRouteV->isDepot())
    {
        auto *node = itRouteV;
        itRouteV = n(itRouteV);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }

    insertLocation = V;
    while (!itRouteU->isDepot())
    {
        auto *node = itRouteU;
        itRouteU = n(itRouteU);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }
}

cost_type TwoOpt::evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V, costEvaluator)
                                : evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Node *U, Node *V) const
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
