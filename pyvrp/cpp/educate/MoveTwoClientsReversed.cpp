#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

cost_type MoveTwoClientsReversed::evaluate(Node *U, Node *V)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto const posU = U->position;
    auto const posV = V->position;

    auto const &dist = data.distanceMatrix();

    auto const current = U->route->distBetween(posU - 1, posU + 2)
                         + dist(V->client, n(V)->client);
    auto const proposed
        = dist(p(U)->client, n(n(U))->client) + dist(V->client, n(U)->client)
          + dist(n(U)->client, U->client) + dist(U->client, n(V)->client);

    cost_type deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(dist, p(U)->twBefore, n(n(U))->twAfter);

        deltaCost += penaltyManager.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= penaltyManager.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + 1);

        deltaCost += penaltyManager.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= penaltyManager.loadPenalty(U->route->load());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += penaltyManager.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= penaltyManager.loadPenalty(V->route->load());

        auto vTWS
            = TWS::merge(dist, V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += penaltyManager.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= penaltyManager.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const uTWS = TWS::merge(dist,
                                         p(U)->twBefore,
                                         route->twBetween(posU + 2, posV),
                                         n(U)->tw,
                                         U->tw,
                                         n(V)->twAfter);

            deltaCost += penaltyManager.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS = TWS::merge(dist,
                                         V->twBefore,
                                         n(U)->tw,
                                         U->tw,
                                         route->twBetween(posV + 1, posU - 1),
                                         n(n(U))->twAfter);

            deltaCost += penaltyManager.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= penaltyManager.twPenalty(route->timeWarp());
    }

    return deltaCost;
}

void MoveTwoClientsReversed::apply(Node *U, Node *V)
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->insertAfter(V);
    X->insertAfter(V);
}
