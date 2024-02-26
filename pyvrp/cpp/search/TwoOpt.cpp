#include "TwoOpt.h"

#include "Route.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::TwoOpt;

Cost TwoOpt::evalWithinRoute(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator) const
{
    assert(U->route() == V->route());
    auto *route = U->route();

    Cost deltaCost
        = -static_cast<Cost>(route->distance())
          - costEvaluator.loadPenalty(route->load(), route->capacity())
          - costEvaluator.twPenalty(route->timeWarp());

    // Current situation is U -> n(U) -> ... -> V -> n(V). Proposed move is
    // U -> V -> p(V) -> ... -> n(U) -> n(V). This reverses the segment from
    // n(U) to V.
    DistanceSegment dist = route->before(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        dist = DistanceSegment::merge(
            data.distanceMatrix(), dist, route->at(idx));
    dist = DistanceSegment::merge(
        data.distanceMatrix(), dist, route->after(V->idx() + 1));

    deltaCost += static_cast<Cost>(dist.distance());

    if (deltaCost >= 0)
        return deltaCost;

    LoadSegment ls = route->before(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        ls = LoadSegment::merge(ls, route->at(idx));
    ls = LoadSegment::merge(ls, route->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());

    DurationSegment ds = route->before(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        ds = DurationSegment::merge(data.durationMatrix(), ds, route->at(idx));
    ds = DurationSegment::merge(
        data.durationMatrix(), ds, route->after(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(ds.timeWarp(route->maxDuration()));

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

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
    }
    else
    {
        auto const uDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     uRoute->before(U->idx()),
                                     uRoute->at(uRoute->size() + 1));

        deltaCost += static_cast<Cost>(uDist.distance());
    }

    if (U->idx() < uRoute->size())
    {
        auto const vDist = DistanceSegment::merge(
            data.distanceMatrix(),
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size()),
            vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
    }
    else
    {
        auto const vDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     vRoute->before(V->idx()),
                                     vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
    }

    deltaCost -= static_cast<Cost>(uRoute->distance());
    deltaCost -= static_cast<Cost>(vRoute->distance());

    if (uRoute->isFeasible() && vRoute->isFeasible() && deltaCost >= 0)
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

    deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());
    deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());

    auto const uLS = LoadSegment::merge(uRoute->before(U->idx()),
                                        vRoute->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(uLS.load(), uRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());

    auto const vLS = LoadSegment::merge(vRoute->before(V->idx()),
                                        uRoute->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(vLS.load(), vRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);

    while (V->idx() > nU->idx())
    {
        auto *pV = p(V);
        Route::swap(nU, V);
        nU = n(V);  // after swap, V is now nU
        V = pV;
    }
}

void TwoOpt::applyBetweenRoutes(Route::Node *U, Route::Node *V) const
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

Cost TwoOpt::evaluate(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator)
{
    assert(U->route());
    assert(V->route());

    if (U->route()->idx() > V->route()->idx())  // tackled in a later iteration
        return 0;

    if (U->route() != V->route())
        return evalBetweenRoutes(U, V, costEvaluator);

    if (U->idx() + 1 >= V->idx())  // tackled in a later iteration
        return 0;

    return evalWithinRoute(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route() == V->route())
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
