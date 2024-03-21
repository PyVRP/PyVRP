#include "SwapStar.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapStar;

void SwapStar::ThreeBest::maybeAdd(Cost costInsert, Route::Node *placeInsert)
{
    if (costInsert >= costs[2])
        return;

    if (costInsert >= costs[1])
    {
        costs[2] = costInsert;
        locs[2] = placeInsert;
    }
    else if (costInsert >= costs[0])
    {
        costs[2] = costs[1];
        locs[2] = locs[1];
        costs[1] = costInsert;
        locs[1] = placeInsert;
    }
    else
    {
        costs[2] = costs[1];
        locs[2] = locs[1];
        costs[1] = costs[0];
        locs[1] = locs[0];
        costs[0] = costInsert;
        locs[0] = placeInsert;
    }
}

void SwapStar::updateRemovalCosts(Route *R, CostEvaluator const &costEvaluator)
{
    updated[R->idx()] = false;

    for (size_t idx = 1; idx != R->size() + 1; ++idx)
    {
        auto const proposal
            = R->proposal(R->before(idx - 1), R->after(idx + 1));

        Cost deltaCost = 0;
        costEvaluator.deltaCost<true, true>(deltaCost, proposal);

        auto const *U = (*R)[idx];
        removalCosts(R->idx(), U->client()) = deltaCost;
    }

    for (size_t idx = data.numDepots(); idx != data.numLocations(); ++idx)
        cache(R->idx(), idx).shouldUpdate = true;
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
        auto const proposal = R->proposal(
            R->before(idx), U->route()->at(U->idx()), R->after(idx + 1));

        Cost deltaCost = 0;
        costEvaluator.deltaCost<true, true>(deltaCost, proposal);

        auto *V = (*R)[idx];
        insertPositions.maybeAdd(deltaCost, V);
    }
}

std::pair<Cost, Route::Node *> SwapStar::getBestInsertPoint(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    auto *route = V->route();
    auto &best_ = cache(route->idx(), U->client());

    if (best_.shouldUpdate)  // then we first update the insert positions
        updateInsertionCost(route, U, costEvaluator);

    for (size_t idx = 0; idx != 3; ++idx)  // only OK if V is not adjacent
        if (best_.locs[idx] && best_.locs[idx] != V && n(best_.locs[idx]) != V)
            return std::make_pair(best_.costs[idx], best_.locs[idx]);

    // As a fallback option, we consider inserting in the place of V.
    Cost deltaCost = 0;
    costEvaluator.deltaCost<true, true>(
        deltaCost,
        route->proposal(route->before(V->idx() - 1),
                        U->route()->at(U->idx()),
                        route->after(V->idx() + 1)));

    return std::make_pair(deltaCost, p(V));
}

Cost SwapStar::evaluateMove(Route::Node const *U,
                            Route::Node const *V,
                            Route::Node const *remove,
                            CostEvaluator const &costEvaluator) const
{
    assert(V->route() == remove->route());
    assert(V != remove);

    auto const *route = V->route();

    Cost deltaCost = 0;

    if (V->idx() + 1 == remove->idx())  // then we insert U in place of remove
        costEvaluator.deltaCost<true>(
            deltaCost,
            route->proposal(route->before(V->idx()),
                            U->route()->at(U->idx()),
                            route->after(V->idx() + 2)));
    else if (V->idx() < remove->idx())
        costEvaluator.deltaCost<true>(
            deltaCost,
            route->proposal(route->before(V->idx()),
                            U->route()->at(U->idx()),
                            route->between(V->idx() + 1, remove->idx() - 1),
                            route->after(remove->idx() + 1)));
    else if (V->idx() > remove->idx())
        costEvaluator.deltaCost<true>(
            deltaCost,
            route->proposal(route->before(remove->idx() - 1),
                            route->between(remove->idx() + 1, V->idx()),
                            U->route()->at(U->idx()),
                            route->after(V->idx() + 1)));

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

    if (updated[routeU->idx()])
        updateRemovalCosts(routeU, costEvaluator);

    if (updated[routeV->idx()])
        updateRemovalCosts(routeV, costEvaluator);

    for (auto *U : *routeU)
        for (auto *V : *routeV)
        {
            // The following lines compute a delta cost of removing U and V from
            // their own routes and inserting them into the other's route in the
            // best place. This is approximate since removal and insertion are
            // evaluated separately, not taking into account that while U leaves
            // its route, V will be inserted (and vice versa).
            Cost deltaCost = 0;

            // Separating removal and insertion means that the effects on load
            // are not counted correctly: during insert, U is still in the
            // route, and now V is added as well. The following addresses this
            // issue with an approximation, which is inexact when there are both
            // pickups and deliveries in the data. We do not evaluate load when
            // calculating remove and insert costs - that is all handled here.
            // So it's pretty rough but fast and seems to work well enough for
            // most instances.
            ProblemData::Client const &uClient = data.location(U->client());
            ProblemData::Client const &vClient = data.location(V->client());
            auto const uLoad = std::max(uClient.delivery, uClient.pickup);
            auto const vLoad = std::max(vClient.delivery, vClient.pickup);
            auto const loadDiff = uLoad - vLoad;

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

    // It is possible for positive delta costs to turn negative when we do an
    // exact evaluation. But in practice that almost never happens, and is not
    // worth spending time on.
    if (best.cost >= 0)
        return best.cost;

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
