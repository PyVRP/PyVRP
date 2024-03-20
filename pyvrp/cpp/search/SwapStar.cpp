#include "SwapStar.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapStar;

void SwapStar::updateRemovalCosts(Route *R, CostEvaluator const &costEvaluator)
{
    updated[R->idx()] = false;

    for (auto *U : *R)
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost<true>(
            R->proposal(R->before(U->idx() - 1), R->after(U->idx() + 1)),
            deltaCost);

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
        auto *V = (*R)[idx];

        Cost deltaCost = 0;
        costEvaluator.deltaCost<true>(R->proposal(R->before(V->idx()),
                                                  U->route()->at(U->idx()),
                                                  R->after(V->idx() + 1)),
                                      deltaCost);

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
    costEvaluator.deltaCost<true>(route->proposal(route->before(V->idx() - 1),
                                                  U->route()->at(U->idx()),
                                                  route->after(V->idx() + 1)),
                                  deltaCost);

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

    if (V == p(remove))  // special case where we insert U in place of remove
        costEvaluator.deltaCost<true>(
            route->proposal(route->before(V->idx()),
                            U->route()->at(U->idx()),
                            route->after(V->idx() + 2)),
            deltaCost);
    else if (V->idx() < remove->idx())
        costEvaluator.deltaCost<true>(
            route->proposal(route->before(V->idx()),
                            U->route()->at(U->idx()),
                            route->between(V->idx() + 1, remove->idx() - 1),
                            route->after(remove->idx() + 1)),
            deltaCost);
    else if (V->idx() > remove->idx())
        costEvaluator.deltaCost<true>(
            route->proposal(route->before(remove->idx() - 1),
                            route->between(remove->idx() + 1, V->idx()),
                            U->route()->at(U->idx()),
                            route->after(V->idx() + 1)),
            deltaCost);

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
            Cost deltaCost = 0;

            deltaCost += removalCosts(routeU->idx(), U->client());
            deltaCost += removalCosts(routeV->idx(), V->client());

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

    // Now do an exact evaluation of the proposed swap move. This includes
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
