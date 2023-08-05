#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

#include <cassert>

using pyvrp::search::MoveTwoClientsReversed;
using TWS = pyvrp::TimeWindowSegment;

pyvrp::Cost MoveTwoClientsReversed::evaluate(
    Route::Node *U, Route::Node *V, pyvrp::CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    assert(U->route() && V->route());
    auto *uRoute = U->route();
    auto *vRoute = V->route();

    Distance const current = uRoute->distBetween(U->idx() - 1, U->idx() + 2)
                             + data.dist(V->client(), n(V)->client());
    Distance const proposed = data.dist(p(U)->client(), n(n(U))->client())
                              + data.dist(V->client(), n(U)->client())
                              + data.dist(n(U)->client(), U->client())
                              + data.dist(U->client(), n(V)->client());

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (uRoute != vRoute)
    {
        if (uRoute->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS
            = TWS::merge(data.durationMatrix(),
                         uRoute->twBetween(0, U->idx() - 1),
                         uRoute->twBetween(U->idx() + 2, uRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());

        auto const loadDiff = uRoute->loadBetween(U->idx(), U->idx() + 1);

        deltaCost += costEvaluator.loadPenalty(uRoute->load() - loadDiff,
                                               uRoute->capacity());
        deltaCost
            -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += costEvaluator.loadPenalty(vRoute->load() + loadDiff,
                                               vRoute->capacity());
        deltaCost
            -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

        auto vTWS
            = TWS::merge(data.durationMatrix(),
                         vRoute->twBetween(0, V->idx()),
                         n(U)->tws(),
                         U->tws(),
                         vRoute->twBetween(V->idx() + 1, vRoute->size() + 1));

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());
    }
    else  // within same route
    {
        auto const *route = uRoute;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (U->idx() < V->idx())
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             route->twBetween(0, U->idx() - 1),
                             route->twBetween(U->idx() + 2, V->idx()),
                             n(U)->tws(),
                             U->tws(),
                             route->twBetween(V->idx() + 1, route->size() + 1));

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             route->twBetween(0, V->idx()),
                             n(U)->tws(),
                             U->tws(),
                             route->twBetween(V->idx() + 1, U->idx() - 1),
                             route->twBetween(U->idx() + 2, route->size() + 1));

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= costEvaluator.twPenalty(route->timeWarp());
    }

    return deltaCost;
}

void MoveTwoClientsReversed::apply(Route::Node *U, Route::Node *V) const
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->route()->remove(X->idx());
    U->route()->remove(U->idx());

    V->route()->insert(V->idx() + 1, U);
    V->route()->insert(V->idx() + 1, X);
}
