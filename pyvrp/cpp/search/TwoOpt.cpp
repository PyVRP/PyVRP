#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using pyvrp::Cost;
using pyvrp::search::TwoOpt;
using TWS = pyvrp::TimeWindowSegment;

Cost TwoOpt::evalWithinRoute(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator) const
{
    if (U->idx() + 1 >= V->idx())
        return 0;

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
                               - U->route()->distBetween(n(U)->idx(), V->idx());

    Cost deltaCost = static_cast<Cost>(deltaDist);

    if (!U->route()->hasTimeWarp() && deltaCost >= 0)
        return deltaCost;

    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(data.durationMatrix(), tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(data.durationMatrix(), tws, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route()->timeWarp());

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    // Two routes. Current situation is U -> n(U), and V -> n(V). Proposed move
    // is U -> n(V) and V -> n(U).
    Distance const current = data.dist(U->client(), n(U)->client())
                             + data.dist(V->client(), n(V)->client());
    Distance const proposed = data.dist(U->client(), n(V)->client())
                              + data.dist(V->client(), n(U)->client());

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route()->isFeasible() && V->route()->isFeasible() && deltaCost >= 0)
        return deltaCost;

    auto const uTWS
        = TWS::merge(data.durationMatrix(), U->twBefore, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route()->timeWarp());

    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route()->timeWarp());

    // Proposed move appends the segment after V to U, and the segment after U
    // to V. So we need to make a distinction between the loads at U and V, and
    // the loads from clients visited after these nodes.
    auto const uLoad = U->route()->loadBetween(0, U->idx());
    auto const uLoadAfter = U->route()->load() - uLoad;
    auto const vLoad = V->route()->loadBetween(0, V->idx());
    auto const vLoadAfter = V->route()->load() - vLoad;

    deltaCost += costEvaluator.loadPenalty(uLoad + vLoadAfter,
                                           U->route()->capacity());
    deltaCost -= costEvaluator.loadPenalty(U->route()->load(),
                                           U->route()->capacity());

    deltaCost += costEvaluator.loadPenalty(vLoad + uLoadAfter,
                                           V->route()->capacity());
    deltaCost -= costEvaluator.loadPenalty(V->route()->load(),
                                           V->route()->capacity());

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

    if (U->route() == V->route())
        return evalWithinRoute(U, V, costEvaluator);
    else
        return evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route() == V->route())
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
