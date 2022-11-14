#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

int TwoOpt::evalWithinRoute(Node *U, Node *V)
{
    if (U->position + 1 >= V->position)
        return 0;

    int deltaCost = d_params.dist(U->client, V->client)
                    + d_params.dist(n(U)->client, n(V)->client)
                    + V->cumulatedReversalDistance
                    - d_params.dist(U->client, n(U)->client)
                    - d_params.dist(V->client, n(V)->client)
                    - n(U)->cumulatedReversalDistance;

    if (!U->route->hasTimeWarp() && deltaCost >= 0)
        return deltaCost;

    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(tws, n(V)->twAfter);

    deltaCost += d_params.twPenalty(tws.totalTimeWarp());
    deltaCost -= d_params.twPenalty(U->route->timeWarp());

    return deltaCost;
}

int TwoOpt::evalBetweenRoutes(Node *U, Node *V)
{
    int const current = d_params.dist(U->client, n(U)->client)
                        + d_params.dist(V->client, n(V)->client);
    int const proposed = d_params.dist(U->client, n(V)->client)
                         + d_params.dist(V->client, n(U)->client);

    int deltaCost = proposed - current;

    if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
        return deltaCost;

    auto const uTWS = TWS::merge(U->twBefore, n(V)->twAfter);

    deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= d_params.twPenalty(U->route->timeWarp());

    auto const vTWS = TWS::merge(V->twBefore, n(U)->twAfter);

    deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= d_params.twPenalty(V->route->timeWarp());

    int const deltaLoad = U->cumulatedLoad - V->cumulatedLoad;

    deltaCost += d_params.loadPenalty(U->route->load() - deltaLoad);
    deltaCost -= d_params.loadPenalty(U->route->load());

    deltaCost += d_params.loadPenalty(V->route->load() + deltaLoad);
    deltaCost -= d_params.loadPenalty(V->route->load());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Node *U, Node *V)
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

void TwoOpt::applyBetweenRoutes(Node *U, Node *V)
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

int TwoOpt::evaluate(Node *U, Node *V)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V)
                                : evalBetweenRoutes(U, V);
}

void TwoOpt::apply(Node *U, Node *V)
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
