#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

#include <cassert>

using TWS = TimeWindowSegment;

Cost MoveTwoClientsReversed::evaluate(Node *U,
                                      Node *V,
                                      CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto const posU = U->position;
    auto const posV = V->position;

    assert(U->route && V->route);

    auto const deltaDistU = data.dist(p(U)->client, n(n(U))->client)
                            - U->route->distBetween(posU - 1, posU + 2);
    auto const deltaDistV = data.dist(V->client, n(U)->client)
                            + data.dist(n(U)->client, U->client)
                            + data.dist(U->client, n(V)->client)
                            - data.dist(V->client, n(V)->client);

    if (U->route != V->route)
    {
        auto const currentCost = U->route->penalisedCost(costEvaluator)
                                 + V->route->penalisedCost(costEvaluator);

        // Compute lower bound for new cost based on distance and load
        auto const distU = U->route->dist() + deltaDistU;
        auto const distV = V->route->dist() + deltaDistV;

        auto const deltaLoad = U->route->loadBetween(posU, posU + 1);
        auto const loadU = U->route->load() - deltaLoad;
        auto const loadV = V->route->load() + deltaLoad;

        auto const lbCostU = costEvaluator.penalisedRouteCost(
            distU, loadU, 0, 0, U->route->vehicleType());
        auto const lbCostV = costEvaluator.penalisedRouteCost(
            distV, loadV, 0, 0, V->route->vehicleType());

        if (lbCostU + lbCostV >= currentCost)
            return 0;

        // Add timing information for route to get actual cost
        auto uTWS = TWS::merge(
            data.durationMatrix(), p(U)->twBefore, n(n(U))->twAfter);
        auto const costU = costEvaluator.penalisedRouteCost(
            distU, loadU, uTWS, U->route->vehicleType());

        // Small optimization, check intermediate bound
        if (costU + lbCostV >= currentCost)
            return 0;

        // Add time warp and actual duration for route V to get actual cost
        auto vTWS = TWS::merge(
            data.durationMatrix(), V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);
        auto const costV = costEvaluator.penalisedRouteCost(
            distV, loadV, vTWS, V->route->vehicleType());

        return costU + costV - currentCost;
    }
    else  // within same route
    {
        auto const *route = U->route;
        auto const currentCost = route->penalisedCost(costEvaluator);
        auto const dist = route->dist() + deltaDistU + deltaDistV;

        // First compute bound based on dist and load
        auto const lbCost = costEvaluator.penalisedRouteCost(
            dist, route->load(), 0, 0, route->vehicleType());
        if (lbCost >= currentCost)
            return 0;

        // Add timing information for route to get actual cost
        auto const tws = (posU < posV)
                             ? TWS::merge(data.durationMatrix(),
                                          p(U)->twBefore,
                                          route->twBetween(posU + 2, posV),
                                          n(U)->tw,
                                          U->tw,
                                          n(V)->twAfter)
                             : TWS::merge(data.durationMatrix(),
                                          V->twBefore,
                                          n(U)->tw,
                                          U->tw,
                                          route->twBetween(posV + 1, posU - 1),
                                          n(n(U))->twAfter);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, route->load(), tws, route->vehicleType());
        return cost - currentCost;
    }
}

void MoveTwoClientsReversed::apply(Node *U, Node *V) const
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->insertAfter(V);
    X->insertAfter(V);
}
