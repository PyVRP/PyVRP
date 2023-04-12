#include "SwapStar.h"

using TWS = TimeWindowSegment;

void SwapStar::updateRemovalCosts(Route *R1, CostEvaluator const &costEvaluator)
{
    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();

    auto const currentCost = R1->penalisedCost(costEvaluator);
    for (Node *U = n(R1->depot); !U->isDepot(); U = n(U))
    {
        auto twData = TWS::merge(durMat, p(U)->twBefore, n(U)->twAfter);
        auto const dist = R1->dist() + distMat(p(U)->client, n(U)->client)
                          - distMat(p(U)->client, U->client)
                          - distMat(U->client, n(U)->client);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, R1->load(), twData.totalTimeWarp(), data.vehicleCapacity());
        removalCosts(R1->idx, U->client) = cost - currentCost;
    }
}

void SwapStar::updateInsertionCost(Route *R,
                                   Node *U,
                                   CostEvaluator const &costEvaluator)
{
    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();
    auto &insertPositions = cache(R->idx, U->client);

    insertPositions = {};
    insertPositions.shouldUpdate = false;

    auto const currentCost = R->penalisedCost(costEvaluator);

    // Insert cost of U just after the depot (0 -> U -> ...)
    auto twData
        = TWS::merge(durMat, R->depot->twBefore, U->tw, n(R->depot)->twAfter);
    auto dist = R->dist() + distMat(0, U->client)
                + distMat(U->client, n(R->depot)->client)
                - distMat(0, n(R->depot)->client);
    auto cost = costEvaluator.penalisedRouteCost(
        dist, R->load(), twData.totalTimeWarp(), data.vehicleCapacity());

    insertPositions.maybeAdd(cost - currentCost, R->depot);

    for (Node *V = n(R->depot); !V->isDepot(); V = n(V))
    {
        // Insert cost of U just after V (V -> U -> ...)
        twData = TWS::merge(durMat, V->twBefore, U->tw, n(V)->twAfter);
        dist = R->dist() + distMat(V->client, U->client)
               + distMat(U->client, n(V)->client)
               - distMat(V->client, n(V)->client);
        cost = costEvaluator.penalisedRouteCost(
            dist, R->load(), twData.totalTimeWarp(), data.vehicleCapacity());
        insertPositions.maybeAdd(cost - currentCost, V);
    }
}

std::pair<cost_type, Node *> SwapStar::getBestInsertPoint(
    Node *U, Node *V, CostEvaluator const &costEvaluator)
{
    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();
    auto &best_ = cache(V->route->idx, U->client);

    if (best_.shouldUpdate)  // then we first update the insert positions
        updateInsertionCost(V->route, U, costEvaluator);

    for (size_t idx = 0; idx != 3; ++idx)  // only OK if V is not adjacent
        if (best_.locs[idx] && best_.locs[idx] != V && n(best_.locs[idx]) != V)
            return std::make_pair(best_.costs[idx], best_.locs[idx]);

    // As a fallback option, we consider inserting in the place of V
    auto const currentCost = V->route->penalisedCost(costEvaluator);
    auto const twData
        = TWS::merge(durMat, p(V)->twBefore, U->tw, n(V)->twAfter);
    auto const dist = V->route->dist() + distMat(p(V)->client, U->client)
                      + distMat(U->client, n(V)->client)
                      - distMat(p(V)->client, n(V)->client);
    auto const cost = costEvaluator.penalisedRouteCost(
        dist, V->route->load(), twData.totalTimeWarp(), data.vehicleCapacity());

    return std::make_pair(cost - currentCost, p(V));
}

cost_type SwapStar::evaluateRouteCost(Node *V,
                                      Node *U,
                                      Node *UAfter,
                                      CostEvaluator const &costEvaluator)
{
    auto const route = V->route;
    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();

    int const uDemand = data.client(U->client).demand;
    int const vDemand = data.client(V->client).demand;
    int const load = route->load() + uDemand - vDemand;

    if (UAfter == p(V))
    {
        // Special case: insert U in place of V
        // Remove p(V) -> V -> n(V)
        // Add p(V) -> U -> n(V)
        auto const deltaDist = distMat(p(V)->client, U->client)
                               + distMat(U->client, n(V)->client)
                               - distMat(p(V)->client, V->client)
                               - distMat(V->client, n(V)->client);
        auto const dist = route->dist() + deltaDist;
        auto const tws
            = TWS::merge(durMat, UAfter->twBefore, U->tw, n(V)->twAfter);

        return costEvaluator.penalisedRouteCost(
            dist, load, tws.totalTimeWarp(), data.vehicleCapacity());
    }
    else
    {
        // Removal of V and insertion of U is in independent parts
        // For V, remove p(V) -> V -> n(V) and add p(V) -> n(V)
        // For U, remove UAfter -> n(UAfter) and add UAfter -> U -> n(UAfter)
        auto const deltaDistV = distMat(p(V)->client, n(V)->client)
                                - distMat(p(V)->client, V->client)
                                - distMat(V->client, n(V)->client);
        auto const deltaDistU = distMat(UAfter->client, U->client)
                                + distMat(U->client, n(UAfter)->client)
                                - distMat(UAfter->client, n(UAfter)->client);
        auto const dist = route->dist() + deltaDistV + deltaDistU;

        // For the time window segment, evaluation depends on whether insertion
        // of U comes before or after removal of V in the route
        auto const tws = (UAfter->position < V->position)
                             ? TWS::merge(durMat,
                                          UAfter->twBefore,
                                          U->tw,
                                          route->twBetween(UAfter->position + 1,
                                                           V->position - 1),
                                          n(V)->twAfter)
                             : TWS::merge(durMat,
                                          p(V)->twBefore,
                                          route->twBetween(V->position + 1,
                                                           UAfter->position),
                                          U->tw,
                                          n(UAfter)->twAfter);
        return costEvaluator.penalisedRouteCost(
            dist, load, tws.totalTimeWarp(), data.vehicleCapacity());
    }
}

void SwapStar::init(Individual const &indiv)
{
    LocalSearchOperator<Route>::init(indiv);
    std::fill(updated.begin(), updated.end(), true);
}

cost_type SwapStar::evaluate(Route *routeU,
                             Route *routeV,
                             CostEvaluator const &costEvaluator)
{
    best = {};

    if (updated[routeV->idx])
    {
        updateRemovalCosts(routeV, costEvaluator);
        updated[routeV->idx] = false;

        for (size_t idx = 1; idx != data.numClients() + 1; ++idx)
            cache(routeV->idx, idx).shouldUpdate = true;
    }

    if (updated[routeU->idx])
    {
        updateRemovalCosts(routeU, costEvaluator);
        updated[routeV->idx] = false;

        for (size_t idx = 1; idx != data.numClients() + 1; ++idx)
            cache(routeU->idx, idx).shouldUpdate = true;
    }

    for (Node *U = n(routeU->depot); !U->isDepot(); U = n(U))
        for (Node *V = n(routeV->depot); !V->isDepot(); V = n(V))
        {
            cost_type deltaCost = 0;

            int const uDemand = data.client(U->client).demand;
            int const vDemand = data.client(V->client).demand;
            int const deltaLoad = uDemand - vDemand;

            deltaCost += costEvaluator.loadPenalty(routeU->load() - deltaLoad,
                                                   data.vehicleCapacity());
            deltaCost -= costEvaluator.loadPenalty(routeU->load(),
                                                   data.vehicleCapacity());

            deltaCost += costEvaluator.loadPenalty(routeV->load() + deltaLoad,
                                                   data.vehicleCapacity());
            deltaCost -= costEvaluator.loadPenalty(routeV->load(),
                                                   data.vehicleCapacity());

            // Note: the removalCosts excludes the change in loadPenalty when
            // removing U, it only considers distance/timewarp
            // which is why we added it here
            deltaCost += removalCosts(routeU->idx, U->client);
            deltaCost += removalCosts(routeV->idx, V->client);

            if (deltaCost >= 0)  // an early filter on many moves, before doing
                continue;        // costly work determining insertion points

            auto [extraV, UAfter] = getBestInsertPoint(U, V, costEvaluator);
            deltaCost += extraV;

            if (deltaCost >= 0)  // continuing here avoids evaluating another
                continue;        // costly insertion point below

            auto [extraU, VAfter] = getBestInsertPoint(V, U, costEvaluator);
            deltaCost += extraU;

            if (deltaCost < best.cost)
            {
                best.cost = deltaCost;

                best.U = U;
                best.UAfter = UAfter;

                best.V = V;
                best.VAfter = VAfter;
            }
        }

    // It is possible for positive delta costs to turn negative when we do a
    // complete evaluation. But in practice that almost never happens, and is
    // not worth spending time on.
    if (best.cost >= 0)
        return best.cost;

    // Now do a full evaluation of the proposed swap move. This includes
    // possible time warp penalties.
    auto const costU
        = evaluateRouteCost(best.U, best.V, best.VAfter, costEvaluator);
    auto const costV
        = evaluateRouteCost(best.V, best.U, best.UAfter, costEvaluator);

    auto const currentCost = routeU->penalisedCost(costEvaluator)
                             + routeV->penalisedCost(costEvaluator);

    return costU + costV - currentCost;
}

void SwapStar::apply(Route *U, Route *V) const
{
    if (best.U && best.UAfter && best.V && best.VAfter)
    {
        best.U->insertAfter(best.UAfter);
        best.V->insertAfter(best.VAfter);
    }
}

void SwapStar::update(Route *U) { updated[U->idx] = true; }
