#include "SwapStar.h"

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapStar;
using TWS = pyvrp::TimeWindowSegment;

void SwapStar::updateRemovalCosts(Route *R1, CostEvaluator const &costEvaluator)
{
    auto const currentCost = R1->penalisedCost(costEvaluator);

    auto const &vehicleType = data.vehicleType(R1->vehicleType());
    for (auto *U : *R1)
    {
        auto const twData
            = TWS::merge(data.durationMatrix(), p(U)->twBefore, n(U)->twAfter);
        auto const dist = R1->dist() + data.dist(p(U)->client, n(U)->client)
                          - data.dist(p(U)->client, U->client)
                          - data.dist(U->client, n(U)->client);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, R1->load(), twData, vehicleType);
        removalCosts(R1->idx, U->client) = cost - currentCost;
    }
}

void SwapStar::updateInsertionCost(Route *R,
                                   Route::Node *U,
                                   CostEvaluator const &costEvaluator)
{
    auto &insertPositions = cache(R->idx, U->client);

    insertPositions = {};
    insertPositions.shouldUpdate = false;

    auto const currentCost = R->penalisedCost(costEvaluator);
    auto const &vehicleType = data.vehicleType(R->vehicleType());

    // Insert cost of U just after the depot (0 -> U -> ...)
    auto twData = TWS::merge(data.durationMatrix(),
                             R->startDepot->twBefore,
                             U->tw,
                             n(R->startDepot)->twAfter);
    auto dist = R->dist() + data.dist(R->startDepot.client, U->client)
                + data.dist(U->client, n(&R->startDepot)->client)
                - data.dist(R->startDepot.client, n(&R->startDepot)->client);
    auto cost = costEvaluator.penalisedRouteCost(
        dist, R->load(), twData, vehicleType);

    insertPositions.maybeAdd(cost - currentCost, &R->startDepot);

    for (auto *V : *R)
    {
        // Insert cost of U just after V (V -> U -> ...)
        twData = TWS::merge(
            data.durationMatrix(), V->twBefore, U->tw, n(V)->twAfter);
        dist = R->dist() + data.dist(V->client, U->client)
               + data.dist(U->client, n(V)->client)
               - data.dist(V->client, n(V)->client);
        cost = costEvaluator.penalisedRouteCost(
            dist, R->load(), twData, vehicleType);
        insertPositions.maybeAdd(cost - currentCost, V);
    }
}

std::pair<Cost, Route::Node *> SwapStar::getBestInsertPoint(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    auto &best_ = cache(V->route->idx, U->client);

    if (best_.shouldUpdate)  // then we first update the insert positions
        updateInsertionCost(V->route, U, costEvaluator);

    for (size_t idx = 0; idx != 3; ++idx)  // only OK if V is not adjacent
        if (best_.locs[idx] && best_.locs[idx] != V && n(best_.locs[idx]) != V)
            return std::make_pair(best_.costs[idx], best_.locs[idx]);

    // As a fallback option, we consider inserting in the place of V
    auto const currentCost = V->route->penalisedCost(costEvaluator);
    auto const &vehicleType = data.vehicleType(V->route->vehicleType());
    auto const twData = TWS::merge(
        data.durationMatrix(), p(V)->twBefore, U->tw, n(V)->twAfter);
    auto const dist = V->route->dist() + data.dist(p(V)->client, U->client)
                      + data.dist(U->client, n(V)->client)
                      - data.dist(p(V)->client, n(V)->client);
    auto const cost = costEvaluator.penalisedRouteCost(
        dist, V->route->load(), twData, vehicleType);

    return std::make_pair(cost - currentCost, p(V));
}

Cost SwapStar::evaluateRouteCost(Node *V,
                                 Node *U,
                                 Node *UAfter,
                                 CostEvaluator const &costEvaluator)
{
    auto const route = V->route;
    auto const &vehicleType = data.vehicleType(route->vehicleType());

    auto const uDemand = data.client(U->client).demand;
    auto const vDemand = data.client(V->client).demand;
    auto const load = route->load() + uDemand - vDemand;

    if (UAfter == p(V))
    {
        // Special case: insert U in place of V
        // Remove p(V) -> V -> n(V)
        // Add p(V) -> U -> n(V)
        auto const deltaDist = data.dist(p(V)->client, U->client)
                               + data.dist(U->client, n(V)->client)
                               - data.dist(p(V)->client, V->client)
                               - data.dist(V->client, n(V)->client);
        auto const dist = route->dist() + deltaDist;
        auto const tws = TWS::merge(
            data.durationMatrix(), UAfter->twBefore, U->tw, n(V)->twAfter);

        return costEvaluator.penalisedRouteCost(dist, load, tws, vehicleType);
    }
    else
    {
        // Removal of V and insertion of U is in independent parts
        // For V, remove p(V) -> V -> n(V) and add p(V) -> n(V)
        // For U, remove UAfter -> n(UAfter) and add UAfter -> U -> n(UAfter)
        auto const deltaDistV = data.dist(p(V)->client, n(V)->client)
                                - data.dist(p(V)->client, V->client)
                                - data.dist(V->client, n(V)->client);
        auto const deltaDistU = data.dist(UAfter->client, U->client)
                                + data.dist(U->client, n(UAfter)->client)
                                - data.dist(UAfter->client, n(UAfter)->client);
        auto const dist = route->dist() + deltaDistV + deltaDistU;

        // For the time window segment, evaluation depends on whether insertion
        // of U comes before or after removal of V in the route
        auto const tws = (UAfter->position < V->position)
                             ? TWS::merge(data.durationMatrix(),
                                          UAfter->twBefore,
                                          U->tw,
                                          route->twBetween(UAfter->position + 1,
                                                           V->position - 1),
                                          n(V)->twAfter)
                             : TWS::merge(data.durationMatrix(),
                                          p(V)->twBefore,
                                          route->twBetween(V->position + 1,
                                                           UAfter->position),
                                          U->tw,
                                          n(UAfter)->twAfter);
        return costEvaluator.penalisedRouteCost(dist, load, tws, vehicleType);
    }
}

void SwapStar::init(Solution const &solution)
{
    LocalSearchOperator<Route>::init(solution);
    std::fill(updated.begin(), updated.end(), true);
}

Cost SwapStar::evaluate(Route *routeU,
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

    for (auto *U : *routeU)
        for (auto *V : *routeV)
        {
            Cost deltaCost = 0;

            auto const uDemand = data.client(U->client).demand;
            auto const vDemand = data.client(V->client).demand;
            auto const deltaLoad = uDemand - vDemand;

            deltaCost += costEvaluator.loadPenalty(routeU->load() - deltaLoad,
                                                   U->route->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeU->load(),
                                                   U->route->capacity());

            deltaCost += costEvaluator.loadPenalty(routeV->load() + deltaLoad,
                                                   V->route->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeV->load(),
                                                   V->route->capacity());

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

void SwapStar::apply([[maybe_unused]] Route *U, [[maybe_unused]] Route *V) const
{
    if (best.U && best.UAfter && best.V && best.VAfter)
    {
        best.U->insertAfter(best.UAfter);
        best.V->insertAfter(best.VAfter);
    }
}

void SwapStar::update(Route *U) { updated[U->idx] = true; }
