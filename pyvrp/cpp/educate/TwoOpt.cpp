#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

cost_type TwoOpt::evalWithinRoute(Node *U,
                                  Node *V,
                                  CostEvaluator const &costEvaluator) const
{
    if (U->position + 1 >= V->position)
        return 0;

    auto const &dist = data.distanceMatrix();
    auto const &duration = data.durationMatrix();

    cost_type deltaCost
        = dist(U->client, V->client) + dist(n(U)->client, n(V)->client)
          + V->cumDeltaRevDist - dist(U->client, n(U)->client)
          - dist(V->client, n(V)->client) - n(U)->cumDeltaRevDist;

    if (!U->route->hasTimeWarp() && deltaCost >= 0)
        return deltaCost;

    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(duration, tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(duration, tws, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    return deltaCost;
}

cost_type TwoOpt::evalBetweenRoutes(Node *U,
                                    Node *V,
                                    CostEvaluator const &costEvaluator) const
{
    auto const &dist = data.distanceMatrix();
    auto const &duration = data.durationMatrix();

    auto const current
        = dist(U->client, n(U)->client) + dist(V->client, n(V)->client);
    auto const proposed
        = dist(U->client, n(V)->client) + dist(V->client, n(U)->client);

    cost_type deltaCost = proposed - current;

    if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
        return deltaCost;

    auto const uTWS = TWS::merge(duration, U->twBefore, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    auto const vTWS = TWS::merge(duration, V->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

    int const deltaLoad = U->cumLoad - V->cumLoad;

    deltaCost += costEvaluator.loadPenalty(U->route->load() - deltaLoad,
                                           data.vehicleCapacity());
    deltaCost
        -= costEvaluator.loadPenalty(U->route->load(), data.vehicleCapacity());

    deltaCost += costEvaluator.loadPenalty(V->route->load() + deltaLoad,
                                           data.vehicleCapacity());
    deltaCost
        -= costEvaluator.loadPenalty(V->route->load(), data.vehicleCapacity());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Node *U, Node *V) const
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

void TwoOpt::applyBetweenRoutes(Node *U, Node *V) const
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

cost_type TwoOpt::evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V, costEvaluator)
                                : evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Node *U, Node *V) const
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
