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

    auto const &distMat = data.distanceMatrix();
    auto const &durMat = data.durationMatrix();

    // Now do a full evaluation of the proposed swap move. This includes
    // possible time warp penalties.
    auto const current = distMat(p(best.U)->client, best.U->client)
                         + distMat(best.U->client, n(best.U)->client)
                         + distMat(p(best.V)->client, best.V->client)
                         + distMat(best.V->client, n(best.V)->client);

    auto const proposed = distMat(best.VAfter->client, best.V->client)
                          + distMat(best.UAfter->client, best.U->client);

    cost_type deltaCost = proposed - current;

    if (best.VAfter == p(best.U))
    {
        // Insert in place of U
        deltaCost += distMat(best.V->client, n(best.U)->client);
    }
    else
    {
        deltaCost += distMat(best.V->client, n(best.VAfter)->client)
                     + distMat(p(best.U)->client, n(best.U)->client)
                     - distMat(best.VAfter->client, n(best.VAfter)->client);
    }

    if (best.UAfter == p(best.V))
        // Insert in place of V
        deltaCost += distMat(best.U->client, n(best.V)->client);
    else
        deltaCost += distMat(best.U->client, n(best.UAfter)->client)
                     + distMat(p(best.V)->client, n(best.V)->client)
                     - distMat(best.UAfter->client, n(best.UAfter)->client);

    // It is not possible to have UAfter == V or VAfter == U, so the positions
    // are always strictly different
    if (best.VAfter->position + 1 == best.U->position)
    {
        // Special case
        auto uTWS = TWS::merge(
            durMat, best.VAfter->twBefore, best.V->tw, n(best.U)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }
    else if (best.VAfter->position < best.U->position)
    {
        auto uTWS = TWS::merge(
            durMat,
            best.VAfter->twBefore,
            best.V->tw,
            routeU->twBetween(best.VAfter->position + 1, best.U->position - 1),
            n(best.U)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }
    else
    {
        auto uTWS = TWS::merge(
            durMat,
            p(best.U)->twBefore,
            routeU->twBetween(best.U->position + 1, best.VAfter->position),
            best.V->tw,
            n(best.VAfter)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }

    if (best.UAfter->position + 1 == best.V->position)
    {
        // Special case
        auto vTWS = TWS::merge(
            durMat, best.UAfter->twBefore, best.U->tw, n(best.V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }
    else if (best.UAfter->position < best.V->position)
    {
        auto vTWS = TWS::merge(
            durMat,
            best.UAfter->twBefore,
            best.U->tw,
            routeV->twBetween(best.UAfter->position + 1, best.V->position - 1),
            n(best.V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }
    else
    {
        auto vTWS = TWS::merge(
            durMat,
            p(best.V)->twBefore,
            routeV->twBetween(best.V->position + 1, best.UAfter->position),
            best.U->tw,
            n(best.UAfter)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }

    deltaCost -= costEvaluator.twPenalty(routeU->timeWarp());
    deltaCost -= costEvaluator.twPenalty(routeV->timeWarp());

    auto const uDemand = data.client(best.U->client).demand;
    auto const vDemand = data.client(best.V->client).demand;

    deltaCost += costEvaluator.loadPenalty(routeU->load() - uDemand + vDemand,
                                           data.vehicleCapacity());
    deltaCost
        -= costEvaluator.loadPenalty(routeU->load(), data.vehicleCapacity());

    deltaCost += costEvaluator.loadPenalty(routeV->load() + uDemand - vDemand,
                                           data.vehicleCapacity());
    deltaCost
        -= costEvaluator.loadPenalty(routeV->load(), data.vehicleCapacity());

    return deltaCost;
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
