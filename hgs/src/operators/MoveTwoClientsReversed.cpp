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
                        + d_params.dist(V->client, n(V)->client);
    int const proposed
        = d_params.dist(p(U)->client, nn(U)->client)
          + d_params.dist(V->client, n(U)->client, U->client, n(V)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(p(U)->twBefore, nn(U)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + 1);

        deltaCost += d_params.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= d_params.loadPenalty(U->route->load());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += d_params.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= d_params.loadPenalty(V->route->load());

        auto vTWS = TWS::merge(V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(V->route->timeWarp());
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

            deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS = TWS::merge(V->twBefore,
                                         n(U)->tw,
                                         U->tw,
                                         route->twBetween(posV + 1, posU - 1),
                                         nn(U)->twAfter);

            deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= d_params.twPenalty(route->timeWarp());
    }

    return deltaCost;
}
