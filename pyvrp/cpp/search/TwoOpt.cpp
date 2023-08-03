#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using pyvrp::Cost;
using pyvrp::search::TwoOpt;
using TWS = pyvrp::TimeWindowSegment;

Cost TwoOpt::evalWithinRoute(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator) const
{
    if (U->position + 1 >= V->position)
        return 0;

    auto const *route = U->route;
    auto const &vehicleType = data.vehicleType(route->vehicleType());
    auto const currentCost = route->penalisedCost(costEvaluator);

    // Current situation is U -> n(U) -> ... -> V -> n(V). Proposed move is
    // U -> V -> p(V) -> ... -> n(U) -> n(V). This reverses the segment from
    // n(U) to V.
    Distance segmentReversalDistance = 0;  // reversal dist of n(U) -> ... -> V
    for (auto *node = V; node != n(U); node = p(node))
        segmentReversalDistance += data.dist(node->client, p(node)->client);

    auto const deltaDist = data.dist(U->client, V->client)
                           + data.dist(n(U)->client, n(V)->client)
                           + segmentReversalDistance
                           - data.dist(U->client, n(U)->client)
                           - data.dist(V->client, n(V)->client)
                           - U->route->distBetween(n(U)->position, V->position);

    // First compute bound based on dist and load
    auto const dist = route->dist() + deltaDist;
    auto const lbCost = costEvaluator.penalisedRouteCost(
        dist, route->load(), 0, 0, vehicleType);
    if (lbCost >= currentCost)
        return 0;

    // Compute time warp for route to get actual cost
    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(data.durationMatrix(), tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(data.durationMatrix(), tws, n(V)->twAfter);

    auto const cost = costEvaluator.penalisedRouteCost(
        dist, route->load(), tws, vehicleType);
    return cost - currentCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    // Two routes. Current situation is U -> n(U), and V -> n(V). Proposed move
    // is U -> n(V) and V -> n(U).
    auto const currentCost = U->route->penalisedCost(costEvaluator)
                             + V->route->penalisedCost(costEvaluator);

    auto const &vehicleTypeU = data.vehicleType(U->route->vehicleType());
    auto const &vehicleTypeV = data.vehicleType(V->route->vehicleType());

    // Compute lower bound for new cost based on distance and load
    auto const distU = U->cumulatedDistance + data.dist(U->client, n(V)->client)
                       + V->route->dist() - n(V)->cumulatedDistance;
    auto const distV = V->cumulatedDistance + data.dist(V->client, n(U)->client)
                       + U->route->dist() - n(U)->cumulatedDistance;

    // Proposed move appends the segment after V to U, and the segment after U
    // to V. So we need to make a distinction between the loads at U and V, and
    // the loads from clients visited after these nodes.
    auto const curLoadUntilU = U->route->loadBetween(0, U->position);
    auto const curLoadAfterU = U->route->load() - curLoadUntilU;
    auto const curLoadUntilV = V->route->loadBetween(0, V->position);
    auto const curLoadAfterV = V->route->load() - curLoadUntilV;

    // Load for new routes
    auto const loadU = curLoadUntilU + curLoadAfterV;
    auto const loadV = curLoadUntilV + curLoadAfterU;

    auto const lbCostU
        = costEvaluator.penalisedRouteCost(distU, loadU, 0, 0, vehicleTypeU);
    auto const lbCostV
        = costEvaluator.penalisedRouteCost(distV, loadV, 0, 0, vehicleTypeV);

    if (lbCostU + lbCostV >= currentCost)
        return 0;

    // Add time warp for route U to get actual cost
    auto const uTWS
        = TWS::merge(data.durationMatrix(), U->twBefore, n(V)->twAfter);
    auto const costU
        = costEvaluator.penalisedRouteCost(distU, loadU, uTWS, vehicleTypeU);

    if (costU + lbCostV >= currentCost)
        return 0;

    // Add time warp for route V to get actual cost
    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, n(U)->twAfter);
    auto const costV
        = costEvaluator.penalisedRouteCost(distV, loadV, vTWS, vehicleTypeV);

    return costU + costV - currentCost;
}

void TwoOpt::applyWithinRoute(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);

    auto insertPos = U->position + 1;
    while (V != nU)
    {
        auto *node = V;
        V = p(V);
        U->route->remove(node->position);
        U->route->insert(insertPos++, node);
    }
}

void TwoOpt::applyBetweenRoutes(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertPos = U->position + 1;
    while (!nV->isDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route->remove(node->position);
        U->route->insert(insertPos++, node);
    }

    insertPos = V->position + 1;
    while (!nU->isDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route->remove(node->position);
        V->route->insert(insertPos++, node);
    }
}

Cost TwoOpt::evaluate(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V, costEvaluator)
                                : evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
