#include "MoveTwoClientsReversed.h"
#include "Route.h"

using pyvrp::search::MoveTwoClientsReversed;

pyvrp::Cost MoveTwoClientsReversed::evaluate(
    Route::Node *U, Route::Node *V, pyvrp::CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    auto *uRoute = U->route();
    auto *vRoute = V->route();

    Cost deltaCost = 0;

    if (uRoute != vRoute)
    {
        auto const uDist = DistanceSegment::merge(data.distanceMatrix(),
                                                  uRoute->before(U->idx() - 1),
                                                  uRoute->after(U->idx() + 2));

        deltaCost += static_cast<Cost>(uDist.distance());
        deltaCost -= static_cast<Cost>(uRoute->distance());

        auto vDist = DistanceSegment::merge(data.distanceMatrix(),
                                            vRoute->before(V->idx()),
                                            uRoute->at(U->idx() + 1),
                                            uRoute->at(U->idx()),
                                            vRoute->after(V->idx() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
        deltaCost -= static_cast<Cost>(vRoute->distance());

        // We're going to incur V's fixed cost if V is currently empty. We lose
        // U's fixed cost if we're moving all of U's clients with this operator.
        deltaCost += Cost(vRoute->empty()) * vRoute->fixedVehicleCost();
        deltaCost -= Cost(uRoute->size() == 2) * uRoute->fixedVehicleCost();

        if (uRoute->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uDS = DurationSegment::merge(data.durationMatrix(),
                                          uRoute->before(U->idx() - 1),
                                          uRoute->after(U->idx() + 2));

        deltaCost
            += costEvaluator.twPenalty(uDS.timeWarp(uRoute->maxDuration()));
        deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());

        auto const uLS = LoadSegment::merge(uRoute->before(U->idx() - 1),
                                            uRoute->after(U->idx() + 2));

        deltaCost += costEvaluator.loadPenalty(uLS.load(), uRoute->capacity());
        deltaCost
            -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        auto const vLS = LoadSegment::merge(vRoute->before(V->idx()),
                                            uRoute->at(U->idx() + 1),
                                            uRoute->at(U->idx()),
                                            vRoute->after(V->idx() + 1));

        deltaCost += costEvaluator.loadPenalty(vLS.load(), vRoute->capacity());
        deltaCost
            -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

        auto vDS = DurationSegment::merge(data.durationMatrix(),
                                          vRoute->before(V->idx()),
                                          uRoute->at(U->idx() + 1),
                                          uRoute->at(U->idx()),
                                          vRoute->after(V->idx() + 1));

        deltaCost
            += costEvaluator.twPenalty(vDS.timeWarp(vRoute->maxDuration()));
        deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());
    }
    else  // within same route
    {
        deltaCost -= static_cast<Cost>(uRoute->distance());
        deltaCost
            -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());
        deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());

        if (U->idx() < V->idx())
        {
            auto const dist = DistanceSegment::merge(
                data.distanceMatrix(),
                uRoute->before(U->idx() - 1),
                uRoute->between(U->idx() + 2, V->idx()),
                uRoute->at(U->idx() + 1),
                uRoute->at(U->idx()),
                uRoute->after(V->idx() + 1));

            deltaCost += static_cast<Cost>(dist.distance());

            auto const ls
                = LoadSegment::merge(uRoute->before(U->idx() - 1),
                                     uRoute->between(U->idx() + 2, V->idx()),
                                     uRoute->at(U->idx() + 1),
                                     uRoute->at(U->idx()),
                                     uRoute->after(V->idx() + 1));

            deltaCost
                += costEvaluator.loadPenalty(ls.load(), uRoute->capacity());

            auto const ds = DurationSegment::merge(
                data.durationMatrix(),
                uRoute->before(U->idx() - 1),
                uRoute->between(U->idx() + 2, V->idx()),
                uRoute->at(U->idx() + 1),
                uRoute->at(U->idx()),
                uRoute->after(V->idx() + 1));

            deltaCost
                += costEvaluator.twPenalty(ds.timeWarp(uRoute->maxDuration()));
        }
        else
        {
            auto const dist = DistanceSegment::merge(
                data.distanceMatrix(),
                uRoute->before(V->idx()),
                uRoute->at(U->idx() + 1),
                uRoute->at(U->idx()),
                uRoute->between(V->idx() + 1, U->idx() - 1),
                uRoute->after(U->idx() + 2));

            deltaCost += static_cast<Cost>(dist.distance());

            auto const ls = LoadSegment::merge(
                uRoute->before(V->idx()),
                uRoute->at(U->idx() + 1),
                uRoute->at(U->idx()),
                uRoute->between(V->idx() + 1, U->idx() - 1),
                uRoute->after(U->idx() + 2));

            deltaCost
                += costEvaluator.loadPenalty(ls.load(), uRoute->capacity());

            auto const ds = DurationSegment::merge(
                data.durationMatrix(),
                uRoute->before(V->idx()),
                uRoute->at(U->idx() + 1),
                uRoute->at(U->idx()),
                uRoute->between(V->idx() + 1, U->idx() - 1),
                uRoute->after(U->idx() + 2));

            deltaCost
                += costEvaluator.twPenalty(ds.timeWarp(uRoute->maxDuration()));
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
