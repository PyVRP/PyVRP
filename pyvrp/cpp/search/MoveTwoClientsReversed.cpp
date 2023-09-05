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

    // We're going to incur V's fixed cost if V is currently empty. We lose U's
    // fixed cost if we're moving all of U's clients with this operator.
    deltaCost += Cost(vRoute->empty()) * vRoute->fixedCost();
    deltaCost -= Cost(uRoute->size() == 2) * uRoute->fixedCost();

    if (uRoute != vRoute)
    {
        if (uRoute->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(data.durationMatrix(),
                               uRoute->twsBefore(U->idx() - 1),
                               uRoute->twsAfter(U->idx() + 2));

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

        auto vTWS = TWS::merge(data.durationMatrix(),
                               vRoute->twsBefore(V->idx()),
                               uRoute->tws(U->idx() + 1),
                               uRoute->tws(U->idx()),
                               vRoute->twsAfter(V->idx() + 1));

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());
    }
    else  // within same route
    {
        deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());

        if (deltaCost >= 0)
            return deltaCost;

        if (U->idx() < V->idx())
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             uRoute->twsBefore(U->idx() - 1),
                             uRoute->twsBetween(U->idx() + 2, V->idx()),
                             uRoute->tws(U->idx() + 1),
                             uRoute->tws(U->idx()),
                             uRoute->twsAfter(V->idx() + 1));

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS
                = TWS::merge(data.durationMatrix(),
                             uRoute->twsBefore(V->idx()),
                             uRoute->tws(U->idx() + 1),
                             uRoute->tws(U->idx()),
                             uRoute->twsBetween(V->idx() + 1, U->idx() - 1),
                             uRoute->twsAfter(U->idx() + 2));

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
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
