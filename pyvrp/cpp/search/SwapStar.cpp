#include "SwapStar.h"

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapStar;
using TWS = pyvrp::TimeWindowSegment;

void SwapStar::updateRemovalCosts(Route *R1, CostEvaluator const &costEvaluator)
{
    for (auto *U : *R1)
    {
        auto twData
            = TWS::merge(data.durationMatrix(), p(U)->twBefore, n(U)->twAfter);

        Distance const deltaDist = data.dist(p(U)->client, n(U)->client)
                                   - data.dist(p(U)->client, U->client)
                                   - data.dist(U->client, n(U)->client);

        removalCosts(R1->idx, U->client)
            = static_cast<Cost>(deltaDist)
              + costEvaluator.twPenalty(twData.totalTimeWarp())
              - costEvaluator.twPenalty(R1->timeWarp());
    }
}

void SwapStar::updateInsertionCost(Route *R,
                                   Route::Node *U,
                                   CostEvaluator const &costEvaluator)
{
    auto &insertPositions = cache(R->idx, U->client);

    insertPositions = {};
    insertPositions.shouldUpdate = false;

    // Insert cost of U just after the depot (0 -> U -> ...)
    auto twData = TWS::merge(data.durationMatrix(),
                             R->startDepot.twBefore,
                             U->tw,
                             n(&R->startDepot)->twAfter);

    Distance deltaDist
        = data.dist(R->startDepot.client, U->client)
          + data.dist(U->client, n(&R->startDepot)->client)
          - data.dist(R->startDepot.client, n(&R->startDepot)->client);

    Cost deltaCost = static_cast<Cost>(deltaDist)
                     + costEvaluator.twPenalty(twData.totalTimeWarp())
                     - costEvaluator.twPenalty(R->timeWarp());

    insertPositions.maybeAdd(deltaCost, &R->startDepot);

    for (auto *V : *R)
    {
        // Insert cost of U just after V (V -> U -> ...)
        twData = TWS::merge(
            data.durationMatrix(), V->twBefore, U->tw, n(V)->twAfter);

        deltaDist = data.dist(V->client, U->client)
                    + data.dist(U->client, n(V)->client)
                    - data.dist(V->client, n(V)->client);

        deltaCost = static_cast<Cost>(deltaDist)
                    + costEvaluator.twPenalty(twData.totalTimeWarp())
                    - costEvaluator.twPenalty(R->timeWarp());

        insertPositions.maybeAdd(deltaCost, V);
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
    auto const twData = TWS::merge(
        data.durationMatrix(), p(V)->twBefore, U->tw, n(V)->twAfter);

    Distance const deltaDist = data.dist(p(V)->client, U->client)
                               + data.dist(U->client, n(V)->client)
                               - data.dist(p(V)->client, n(V)->client);
    Cost const deltaCost = static_cast<Cost>(deltaDist)
                           + costEvaluator.twPenalty(twData.totalTimeWarp())
                           - costEvaluator.twPenalty(V->route->timeWarp());

    return std::make_pair(deltaCost, p(V));
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
            auto const loadDiff = uDemand - vDemand;

            deltaCost += costEvaluator.loadPenalty(routeU->load() - loadDiff,
                                                   U->route->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeU->load(),
                                                   U->route->capacity());

            deltaCost += costEvaluator.loadPenalty(routeV->load() + loadDiff,
                                                   V->route->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeV->load(),
                                                   V->route->capacity());

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
    Distance const current = data.dist(p(best.U)->client, best.U->client)
                             + data.dist(best.U->client, n(best.U)->client)
                             + data.dist(p(best.V)->client, best.V->client)
                             + data.dist(best.V->client, n(best.V)->client);

    Distance const proposed = data.dist(best.VAfter->client, best.V->client)
                              + data.dist(best.UAfter->client, best.U->client);

    Distance deltaDist = proposed - current;

    if (best.VAfter == p(best.U))
        // Insert in place of U
        deltaDist += data.dist(best.V->client, n(best.U)->client);
    else
        deltaDist += data.dist(best.V->client, n(best.VAfter)->client)
                     + data.dist(p(best.U)->client, n(best.U)->client)
                     - data.dist(best.VAfter->client, n(best.VAfter)->client);

    if (best.UAfter == p(best.V))
        // Insert in place of V
        deltaDist += data.dist(best.U->client, n(best.V)->client);
    else
        deltaDist += data.dist(best.U->client, n(best.UAfter)->client)
                     + data.dist(p(best.V)->client, n(best.V)->client)
                     - data.dist(best.UAfter->client, n(best.UAfter)->client);

    Cost deltaCost = static_cast<Cost>(deltaDist);

    // It is not possible to have UAfter == V or VAfter == U, so the positions
    // are always strictly different
    if (best.VAfter->position + 1 == best.U->position)
    {
        // Special case
        auto uTWS = TWS::merge(data.durationMatrix(),
                               best.VAfter->twBefore,
                               best.V->tw,
                               n(best.U)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }
    else if (best.VAfter->position < best.U->position)
    {
        auto uTWS = TWS::merge(
            data.durationMatrix(),
            best.VAfter->twBefore,
            best.V->tw,
            routeU->twBetween(best.VAfter->position + 1, best.U->position - 1),
            n(best.U)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }
    else
    {
        auto uTWS = TWS::merge(
            data.durationMatrix(),
            p(best.U)->twBefore,
            routeU->twBetween(best.U->position + 1, best.VAfter->position),
            best.V->tw,
            n(best.VAfter)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }

    if (best.UAfter->position + 1 == best.V->position)
    {
        // Special case
        auto vTWS = TWS::merge(data.durationMatrix(),
                               best.UAfter->twBefore,
                               best.U->tw,
                               n(best.V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }
    else if (best.UAfter->position < best.V->position)
    {
        auto vTWS = TWS::merge(
            data.durationMatrix(),
            best.UAfter->twBefore,
            best.U->tw,
            routeV->twBetween(best.UAfter->position + 1, best.V->position - 1),
            n(best.V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }
    else
    {
        auto vTWS = TWS::merge(
            data.durationMatrix(),
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
                                           routeU->capacity());
    deltaCost -= costEvaluator.loadPenalty(routeU->load(), routeU->capacity());

    deltaCost += costEvaluator.loadPenalty(routeV->load() + uDemand - vDemand,
                                           routeV->capacity());
    deltaCost -= costEvaluator.loadPenalty(routeV->load(), routeV->capacity());

    return deltaCost;
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
