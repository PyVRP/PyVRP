#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::TwoOpt;
using TWS = pyvrp::TimeWindowSegment;

Cost TwoOpt::evalWithinRoute(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator) const
{
    assert(U->route() == V->route());
    auto *route = U->route();

    // Current situation is U -> n(U) -> ... -> V -> n(V). Proposed move is
    // U -> V -> p(V) -> ... -> n(U) -> n(V). This reverses the segment from
    // n(U) to V.
    Distance segmentReversalDistance = 0;  // reversal dist of n(U) -> ... -> V
    for (auto *node = V; node != n(U); node = p(node))
        segmentReversalDistance += data.dist(node->client(), p(node)->client());

    Distance const deltaDist = data.dist(U->client(), V->client())
                               + data.dist(n(U)->client(), n(V)->client())
                               + segmentReversalDistance
                               - data.dist(U->client(), n(U)->client())
                               - data.dist(V->client(), n(V)->client())
                               - route->distBetween(U->idx() + 1, V->idx());

    Cost deltaCost = static_cast<Cost>(deltaDist);

    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    if (deltaCost >= 0)
        return deltaCost;

    auto tws = route->twsBefore(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        tws = TWS::merge(data.durationMatrix(), tws, route->tws(idx));
    tws = TWS::merge(data.durationMatrix(), tws, route->twsAfter(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    assert(U->route() && V->route());
    auto *uRoute = U->route();
    auto *vRoute = V->route();

    // Two routes. Current situation is U -> n(U), and V -> n(V). Proposed move
    // is U -> n(V) and V -> n(U).
    Distance const current = data.dist(U->client(), n(U)->client())
                             + data.dist(V->client(), n(V)->client());
    Distance const proposed = data.dist(U->client(), n(V)->client())
                              + data.dist(V->client(), n(U)->client());

    Cost deltaCost = static_cast<Cost>(proposed - current);

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    if (uRoute->empty() && U->isDepot() && !n(V)->isDepot())
        deltaCost += uRoute->fixedCost();

    if (vRoute->empty() && V->isDepot() && !n(U)->isDepot())
        deltaCost += vRoute->fixedCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->isDepot() && n(V)->isDepot())
        deltaCost -= uRoute->fixedCost();

    if (!vRoute->empty() && V->isDepot() && n(U)->isDepot())
        deltaCost -= vRoute->fixedCost();

    if (uRoute->isFeasible() && vRoute->isFeasible() && deltaCost >= 0)
        return deltaCost;

    if (V->idx() < vRoute->size())
    {
        auto const uTWS
            = TWS::merge(data.durationMatrix(),
                         uRoute->twsBefore(U->idx()),
                         vRoute->twsBetween(V->idx() + 1, vRoute->size()),
                         uRoute->tws(uRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }
    else
    {
        auto const uTWS = TWS::merge(data.durationMatrix(),
                                     uRoute->twsBefore(U->idx()),
                                     uRoute->tws(uRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    }

    deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());

    if (U->idx() < uRoute->size())
    {
        auto const vTWS
            = TWS::merge(data.durationMatrix(),
                         vRoute->twsBefore(V->idx()),
                         uRoute->twsBetween(U->idx() + 1, uRoute->size()),
                         vRoute->tws(vRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }
    else
    {
        auto const vTWS = TWS::merge(data.durationMatrix(),
                                     vRoute->twsBefore(V->idx()),
                                     vRoute->tws(vRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    }

    deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());

    // Proposed move appends the segment after V to U, and the segment after U
    // to V. So we need to make a distinction between the loads at U and V, and
    // the loads from clients visited after these nodes.
    auto const uLoad = uRoute->loadBetween(0, U->idx());
    auto const uLoadAfter = uRoute->load() - uLoad;
    auto const vLoad = vRoute->loadBetween(0, V->idx());
    auto const vLoadAfter = vRoute->load() - vLoad;

    deltaCost
        += costEvaluator.loadPenalty(uLoad + vLoadAfter, uRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());

    deltaCost
        += costEvaluator.loadPenalty(vLoad + uLoadAfter, vRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);

    while (V->idx() > nU->idx())
    {
        auto *pV = p(V);
        Route::swap(nU, V);
        nU = n(V);  // after swap, V is now nU
        V = pV;
    }
}

void TwoOpt::applyBetweenRoutes(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertIdx = U->idx() + 1;
    while (!nV->isDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdx++, node);
    }

    insertIdx = V->idx() + 1;
    while (!nU->isDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdx++, node);
    }
}

Cost TwoOpt::evaluate(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator)
{
    if (U->route()->idx() > V->route()->idx())  // tackled in a later iteration
        return 0;

    if (U->route() != V->route())
        return evalBetweenRoutes(U, V, costEvaluator);

    if (U->idx() + 1 >= V->idx())  // tackled in a later iteration
        return 0;

    return evalWithinRoute(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route() == V->route())
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
