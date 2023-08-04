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

    Distance const current = U->route()->distBetween(U->idx() - 1, U->idx() + 2)
                             + data.dist(V->client(), n(V)->client());
    Distance const proposed = data.dist(p(U)->client(), n(n(U))->client())
                              + data.dist(V->client(), n(U)->client())
                              + data.dist(n(U)->client(), U->client())
                              + data.dist(U->client(), n(V)->client());

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route() != V->route())
    {
        if (U->route()->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(
            data.durationMatrix(), p(U)->twBefore, n(n(U))->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(U->route()->timeWarp());

        auto const loadDiff = U->route()->loadBetween(U->idx(), U->idx() + 1);

        deltaCost += costEvaluator.loadPenalty(U->route()->load() - loadDiff,
                                               U->route()->capacity());
        deltaCost -= costEvaluator.loadPenalty(U->route()->load(),
                                               U->route()->capacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += costEvaluator.loadPenalty(V->route()->load() + loadDiff,
                                               V->route()->capacity());
        deltaCost -= costEvaluator.loadPenalty(V->route()->load(),
                                               V->route()->capacity());

        auto vTWS = TWS::merge(
            data.durationMatrix(), V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(V->route()->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route();

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (U->idx() < V->idx())
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             p(U)->twBefore,
                             route->twBetween(U->idx() + 2, V->idx()),
                             n(U)->tw,
                             U->tw,
                             n(V)->twAfter);

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             V->twBefore,
                             n(U)->tw,
                             U->tw,
                             route->twBetween(V->idx() + 1, U->idx() - 1),
                             n(n(U))->twAfter);

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
