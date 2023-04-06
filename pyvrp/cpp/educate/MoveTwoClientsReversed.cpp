#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

int MoveTwoClientsReversed::evaluate(Node *U,
                                     Node *V,
                                     CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto const posU = U->position;
    auto const posV = V->position;

    auto const &dist = data.distanceMatrix();

    int const current = U->route->distBetween(posU - 1, posU + 2)
                        + dist(V->client, n(V)->client);
    int const proposed
        = dist(p(U)->client, n(n(U))->client) + dist(V->client, n(U)->client)
          + dist(n(U)->client, U->client) + dist(U->client, n(V)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(dist, p(U)->twBefore, n(n(U))->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + 1);

        deltaCost += costEvaluator.loadPenalty(U->route->load() - loadDiff,
                                               U->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(U->route->load(),
                                               U->route->capacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += costEvaluator.loadPenalty(V->route->load() + loadDiff,
                                               V->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(V->route->load(),
                                               V->route->capacity());

        auto vTWS
            = TWS::merge(dist, V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());
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

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS = TWS::merge(dist,
                                         V->twBefore,
                                         n(U)->tw,
                                         U->tw,
                                         route->twBetween(posV + 1, posU - 1),
                                         n(n(U))->twAfter);

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= costEvaluator.twPenalty(route->timeWarp());
    }

    return deltaCost;
}

void MoveTwoClientsReversed::apply(Node *U, Node *V) const
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->insertAfter(V);
    X->insertAfter(V);
}
