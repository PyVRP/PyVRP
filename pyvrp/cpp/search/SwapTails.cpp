#include "SwapTails.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::SwapTails;

pyvrp::Cost SwapTails::evaluate(Route::Node *U,
                                Route::Node *V,
                                CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (uRoute == vRoute || uRoute->idx() > vRoute->idx())
        return 0;  // same route, or move will be tackled in a later iteration

    Cost deltaCost = 0;

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    if (uRoute->empty() && U->isDepot() && !n(V)->isDepot())
        deltaCost += uRoute->fixedVehicleCost();

    if (vRoute->empty() && V->isDepot() && !n(U)->isDepot())
        deltaCost += vRoute->fixedVehicleCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->isDepot() && n(V)->isDepot())
        deltaCost -= uRoute->fixedVehicleCost();

    if (!vRoute->empty() && V->isDepot() && n(U)->isDepot())
        deltaCost -= vRoute->fixedVehicleCost();

    if (V->idx() < vRoute->size())
    {
        auto const uDist = DistanceSegment::merge(
            data.distanceMatrix(),
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size()),
            uRoute->at(uRoute->size() + 1));

        deltaCost += static_cast<Cost>(uDist.distance());
        deltaCost += costEvaluator.distPenalty(uDist.distance(),
                                               uRoute->maxDistance());
    }
    else
    {
        auto const uDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     uRoute->before(U->idx()),
                                     uRoute->at(uRoute->size() + 1));

        deltaCost += static_cast<Cost>(uDist.distance());
        deltaCost += costEvaluator.distPenalty(uDist.distance(),
                                               uRoute->maxDistance());
    }

    if (U->idx() < uRoute->size())
    {
        auto const vDist = DistanceSegment::merge(
            data.distanceMatrix(),
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size()),
            vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
        deltaCost += costEvaluator.distPenalty(vDist.distance(),
                                               vRoute->maxDistance());
    }
    else
    {
        auto const vDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     vRoute->before(V->idx()),
                                     vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
        deltaCost += costEvaluator.distPenalty(vDist.distance(),
                                               vRoute->maxDistance());
    }

    deltaCost -= static_cast<Cost>(uRoute->distance());
    deltaCost
        -= costEvaluator.distPenalty(uRoute->distance(), uRoute->maxDistance());

    deltaCost -= static_cast<Cost>(vRoute->distance());
    deltaCost
        -= costEvaluator.distPenalty(vRoute->distance(), vRoute->maxDistance());

    deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());
    deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());

    deltaCost -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

    if (deltaCost >= 0)
        return deltaCost;

    if (V->idx() < vRoute->size())
    {
        auto const uDS = DurationSegment::merge(
            data.durationMatrix(),
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size()),
            uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uDS.timeWarp(uRoute->maxDuration()));
    }
    else
    {
        auto const uDS = DurationSegment::merge(data.durationMatrix(),
                                                uRoute->before(U->idx()),
                                                uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uDS.timeWarp(uRoute->maxDuration()));
    }

    if (U->idx() < uRoute->size())
    {
        auto const vDS = DurationSegment::merge(
            data.durationMatrix(),
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size()),
            vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vDS.timeWarp(vRoute->maxDuration()));
    }
    else
    {
        auto const vDS = DurationSegment::merge(data.durationMatrix(),
                                                vRoute->before(V->idx()),
                                                vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vDS.timeWarp(vRoute->maxDuration()));
    }

    auto const uLS = LoadSegment::merge(uRoute->before(U->idx()),
                                        vRoute->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(uLS.load(), uRoute->capacity());

    auto const vLS = LoadSegment::merge(vRoute->before(V->idx()),
                                        uRoute->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(vLS.load(), vRoute->capacity());

    return deltaCost;
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertIdx = U->idx() + 1;
    while (!nV->isDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdx++, node);
    }

    insertIdx = V->idx() + 1;
    while (!nU->isDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdx++, node);
    }
}
