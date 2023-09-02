#include "SwapStar.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapStar;
using TWS = pyvrp::TimeWindowSegment;

void SwapStar::updateRemovalCosts(Route *R1, CostEvaluator const &costEvaluator)
{
    for (auto *U : *R1)
    {
        auto twData = TWS::merge(data.durationMatrix(),
                                 R1->twsBefore(U->idx() - 1),
                                 R1->twsAfter(U->idx() + 1));

        Distance const deltaDist = data.dist(p(U)->client(), n(U)->client())
                                   - data.dist(p(U)->client(), U->client())
                                   - data.dist(U->client(), n(U)->client());

        removalCosts(R1->idx(), U->client())
            = static_cast<Cost>(deltaDist)
              + costEvaluator.twPenalty(twData.totalTimeWarp())
              - costEvaluator.twPenalty(R1->timeWarp());
    }
}

void SwapStar::updateInsertionCost(Route *R,
                                   Route::Node *U,
                                   CostEvaluator const &costEvaluator)
{
    auto &insertPositions = cache(R->idx(), U->client());

    insertPositions = {};
    insertPositions.shouldUpdate = false;

    for (size_t idx = 0; idx != R->size() + 1; ++idx)
    {
        // Insert cost of U just after V (V -> U -> ...).
        auto const tws = TWS::merge(data.durationMatrix(),
                                    R->twsBefore(idx),
                                    U->route()->tws(U->idx()),
                                    R->twsAfter(idx + 1));

        auto *V = (*R)[idx];
        auto const deltaDist = data.dist(V->client(), U->client())
                               + data.dist(U->client(), n(V)->client())
                               - data.dist(V->client(), n(V)->client());

        auto const deltaCost = static_cast<Cost>(deltaDist)
                               + costEvaluator.twPenalty(tws.totalTimeWarp())
                               - costEvaluator.twPenalty(R->timeWarp());

        insertPositions.maybeAdd(deltaCost, V);
    }
}

std::pair<Cost, Route::Node *> SwapStar::getBestInsertPoint(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    auto &best_ = cache(V->route()->idx(), U->client());

    if (best_.shouldUpdate)  // then we first update the insert positions
        updateInsertionCost(V->route(), U, costEvaluator);

    for (size_t idx = 0; idx != 3; ++idx)  // only OK if V is not adjacent
        if (best_.locs[idx] && best_.locs[idx] != V && n(best_.locs[idx]) != V)
            return std::make_pair(best_.costs[idx], best_.locs[idx]);

    // As a fallback option, we consider inserting in the place of V.
    auto const twData = TWS::merge(data.durationMatrix(),
                                   V->route()->twsBefore(V->idx() - 1),
                                   U->route()->tws(U->idx()),
                                   V->route()->twsAfter(V->idx() + 1));

    Distance const deltaDist = data.dist(p(V)->client(), U->client())
                               + data.dist(U->client(), n(V)->client())
                               - data.dist(p(V)->client(), n(V)->client());
    Cost const deltaCost = static_cast<Cost>(deltaDist)
                           + costEvaluator.twPenalty(twData.totalTimeWarp())
                           - costEvaluator.twPenalty(V->route()->timeWarp());

    return std::make_pair(deltaCost, p(V));
}

Cost SwapStar::evaluateMove(Route::Node *U,
                            Route::Node *V,
                            Route::Node *remove,
                            CostEvaluator const &costEvaluator)
{
    assert(V->route() == remove->route());
    assert(V != remove);

    auto const *route = V->route();

    Cost deltaCost = 0;

    if (V == p(remove))
    {
        // Special case: insert U in place of remove. Doing so removes edges
        // V -> remove -> n(remove), and adds V -> U -> n(remove).
        auto const deltaDist
            = data.dist(V->client(), U->client())
              + data.dist(U->client(), n(remove)->client())
              - data.dist(V->client(), remove->client())
              - data.dist(remove->client(), n(remove)->client());

        deltaCost += static_cast<Cost>(deltaDist);

        auto const tws = TWS::merge(data.durationMatrix(),
                                    route->twsBefore(V->idx()),
                                    U->route()->tws(U->idx()),
                                    route->twsAfter(V->idx() + 2));

        deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
    }
    else  // in non-adjacent parts of the route.
    {
        auto const current = data.dist(V->client(), n(V)->client())
                             + data.dist(p(remove)->client(), remove->client())
                             + data.dist(remove->client(), n(remove)->client());

        auto const proposed
            = data.dist(V->client(), U->client())
              + data.dist(U->client(), n(V)->client())
              + data.dist(p(remove)->client(), n(remove)->client());

        deltaCost += static_cast<Cost>(proposed - current);

        if (V->idx() < remove->idx())
        {
            auto const tws
                = TWS::merge(data.durationMatrix(),
                             route->twsBefore(V->idx()),
                             U->route()->tws(U->idx()),
                             route->twsBetween(V->idx() + 1, remove->idx() - 1),
                             route->twsAfter(remove->idx() + 1));

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws
                = TWS::merge(data.durationMatrix(),
                             route->twsBefore(remove->idx() - 1),
                             route->twsBetween(remove->idx() + 1, V->idx()),
                             U->route()->tws(U->idx()),
                             route->twsAfter(V->idx() + 1));

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }
    }

    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    auto const loadDiff = data.client(U->client()).demand
                          - data.client(remove->client()).demand;

    deltaCost += costEvaluator.loadPenalty(route->load() + loadDiff,
                                           route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    return deltaCost;
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

    if (updated[routeV->idx()])
    {
        updateRemovalCosts(routeV, costEvaluator);
        updated[routeV->idx()] = false;

        for (size_t idx = 1; idx != data.numClients() + 1; ++idx)
            cache(routeV->idx(), idx).shouldUpdate = true;
    }

    if (updated[routeU->idx()])
    {
        updateRemovalCosts(routeU, costEvaluator);
        updated[routeV->idx()] = false;

        for (size_t idx = 1; idx != data.numClients() + 1; ++idx)
            cache(routeU->idx(), idx).shouldUpdate = true;
    }

    for (auto *U : *routeU)
        for (auto *V : *routeV)
        {
            Cost deltaCost = 0;

            auto const uDemand = data.client(U->client()).demand;
            auto const vDemand = data.client(V->client()).demand;
            auto const loadDiff = uDemand - vDemand;

            deltaCost += costEvaluator.loadPenalty(routeU->load() - loadDiff,
                                                   routeU->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeU->load(),
                                                   routeU->capacity());

            deltaCost += costEvaluator.loadPenalty(routeV->load() + loadDiff,
                                                   routeV->capacity());
            deltaCost -= costEvaluator.loadPenalty(routeV->load(),
                                                   routeV->capacity());

            deltaCost += removalCosts(routeU->idx(), U->client());
            deltaCost += removalCosts(routeV->idx(), V->client());

            if (deltaCost >= 0)  // an early filter on many moves, before doing
                continue;        // costly work determining insertion points

            auto [extraV, UAfter] = getBestInsertPoint(U, V, costEvaluator);
            deltaCost += extraV;

            if (deltaCost >= 0)  // continuing here avoids evaluating another
                continue;        // potentially costly insertion point below

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
    return evaluateMove(best.V, best.VAfter, best.U, costEvaluator)
           + evaluateMove(best.U, best.UAfter, best.V, costEvaluator);
}

void SwapStar::apply(Route *U, Route *V) const
{
    assert(best.U);
    assert(best.UAfter);
    assert(best.V);
    assert(best.VAfter);

    U->remove(best.U->idx());
    V->remove(best.V->idx());

    V->insert(best.UAfter->idx() + 1, best.U);
    U->insert(best.VAfter->idx() + 1, best.V);
}

void SwapStar::update(Route *U) { updated[U->idx()] = true; }
