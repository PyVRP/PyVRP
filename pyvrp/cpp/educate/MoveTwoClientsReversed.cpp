#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

cost_type MoveTwoClientsReversed::evaluate(Node *U,
                                           Node *V,
                                           CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto const posU = U->position;
    auto const posV = V->position;

    auto const &dist = data.distanceMatrix();
    auto const &duration = data.durationMatrix();

    auto const deltaDistU = dist(p(U)->client, n(n(U))->client)
                            - U->route->distBetween(posU - 1, posU + 2);
    auto const deltaDistV
        = dist(V->client, n(U)->client) + dist(n(U)->client, U->client)
          + dist(U->client, n(V)->client) - dist(V->client, n(V)->client);

    if (U->route != V->route)
    {
        auto const currentCost
            = U->route->penalisedCost() + V->route->penalisedCost();

        // Compute lower bound for new cost based on distance and load
        auto const distU = U->route->dist() + deltaDistU;
        auto const distV = V->route->dist() + deltaDistV;

        auto const loadDiff = U->route->loadBetween(posU, posU + 1);
        auto const loadU = U->route->load() - loadDiff;
        auto const loadV = V->route->load() + loadDiff;

        auto const lbCostU = costEvaluator.penalisedRouteCost(
            distU, loadU, 0, data.vehicleCapacity());
        auto const lbCostV = costEvaluator.penalisedRouteCost(
            distV, loadV, 0, data.vehicleCapacity());

        if (lbCostU + lbCostV >= currentCost)
            return 0;

        // Add time warp for route U to get actual cost
        auto uTWS = TWS::merge(duration, p(U)->twBefore, n(n(U))->twAfter);
        auto const costU = costEvaluator.penalisedRouteCost(
            distU, loadU, uTWS.totalTimeWarp(), data.vehicleCapacity());

        // Small optimization, check intermediate bound
        if (costU + lbCostV >= currentCost)
            return 0;

        // Add time warp for route V to get actual cost
        auto vTWS
            = TWS::merge(duration, V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);
        auto const costV = costEvaluator.penalisedRouteCost(
            distV, loadV, vTWS.totalTimeWarp(), data.vehicleCapacity());

        return costU + costV - currentCost;
    }
    else  // within same route
    {
        // U == V
        auto const dist = U->route->dist() + deltaDistU + deltaDistV;
        auto const *route = U->route;

        // First compute bound based on dist and load
        auto const lbCost = costEvaluator.penalisedRouteCost(
            dist, route->load(), 0, data.vehicleCapacity());
        if (lbCost >= route->penalisedCost())
            return 0;

        // Compute time warp for route to get actual cost
        auto const tws = (posU < posV)
                             ? TWS::merge(duration,
                                          p(U)->twBefore,
                                          route->twBetween(posU + 2, posV),
                                          n(U)->tw,
                                          U->tw,
                                          n(V)->twAfter)
                             : TWS::merge(duration,
                                          V->twBefore,
                                          n(U)->tw,
                                          U->tw,
                                          route->twBetween(posV + 1, posU - 1),
                                          n(n(U))->twAfter);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, route->load(), tws.totalTimeWarp(), data.vehicleCapacity());
        return cost - route->penalisedCost();
    }
}

void MoveTwoClientsReversed::apply(Node *U, Node *V) const
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->insertAfter(V);
    X->insertAfter(V);
}
