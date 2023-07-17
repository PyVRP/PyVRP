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
    if (U->position + 1 >= V->position)
        return 0;

    Distance const deltaDist
        = data.dist(U->client, V->client)
          + data.dist(n(U)->client, n(V)->client) + V->deltaReversalDistance
          - data.dist(U->client, n(U)->client)
          - data.dist(V->client, n(V)->client) - n(U)->deltaReversalDistance;

    Cost deltaCost = static_cast<Cost>(deltaDist);

    if (!U->route->hasTimeWarp() && deltaCost >= 0)
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
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    Distance const current = data.dist(U->client, n(U)->client)
                             + data.dist(V->client, n(V)->client);
    Distance const proposed = data.dist(U->client, n(V)->client)
                              + data.dist(V->client, n(U)->client);

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
        return deltaCost;

    auto const uTWS
        = TWS::merge(data.durationMatrix(), U->twBefore, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

    deltaCost += costEvaluator.loadPenalty(U->cumulatedLoad + V->route->load()
                                               - V->cumulatedLoad,
                                           U->route->capacity());
    deltaCost
        -= costEvaluator.loadPenalty(U->route->load(), U->route->capacity());

    deltaCost += costEvaluator.loadPenalty(V->cumulatedLoad + U->route->load()
                                               - U->cumulatedLoad,
                                           V->route->capacity());
    deltaCost
        -= costEvaluator.loadPenalty(V->route->load(), V->route->capacity());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Route::Node *U, Route::Node *V) const
{
    auto *itRoute = V;
    auto *insertionPoint = U;
    auto *currNext = n(U);

    while (itRoute != currNext)  // No need to move x, we pivot around it
    {
        auto *current = itRoute;
        itRoute = p(itRoute);
        current->insertAfter(insertionPoint);
        insertionPoint = current;
    }
}

void TwoOpt::applyBetweenRoutes(Route::Node *U, Route::Node *V) const
{
    auto *itRouteU = n(U);
    auto *itRouteV = n(V);

    auto *insertLocation = U;
    while (!itRouteV->isDepot())
    {
        auto *node = itRouteV;
        itRouteV = n(itRouteV);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }

    insertLocation = V;
    while (!itRouteU->isDepot())
    {
        auto *node = itRouteU;
        itRouteU = n(itRouteU);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }
}

Cost TwoOpt::evaluate(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V, costEvaluator)
                                : evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
