#include "MoveTwoClientsReversed.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

int MoveTwoClientsReversed::evaluate(Node *U, Node *V)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto const posU = U->position;
    auto const posV = V->position;

    int const current = U->route->distBetween(posU - 1, posU + 2)
                        + data.dist(V->client, n(V)->client);
    int const proposed
        = data.dist(p(U)->client, nn(U)->client)
          + data.dist(V->client, n(U)->client, U->client, n(V)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(p(U)->twBefore, nn(U)->twAfter);

        deltaCost += data.pManager.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= data.pManager.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + 1);

        deltaCost += data.pManager.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= data.pManager.loadPenalty(U->route->load());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += data.pManager.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= data.pManager.loadPenalty(V->route->load());

        auto vTWS = TWS::merge(V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += data.pManager.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= data.pManager.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const uTWS = TWS::merge(p(U)->twBefore,
                                         route->twBetween(posU + 2, posV),
                                         n(U)->tw,
                                         U->tw,
                                         n(V)->twAfter);

            deltaCost += data.pManager.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS = TWS::merge(V->twBefore,
                                         n(U)->tw,
                                         U->tw,
                                         route->twBetween(posV + 1, posU - 1),
                                         nn(U)->twAfter);

            deltaCost += data.pManager.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= data.pManager.twPenalty(route->timeWarp());
    }

    return deltaCost;
}
