#include "ReverseSegment.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::ReverseSegment;

pyvrp::Cost ReverseSegment::evaluate(Route::Node *U,
                                     Route::Node *V,
                                     CostEvaluator const &costEvaluator)
{
    assert(U->route() && V->route());
    auto *route = U->route();

    // Cannot reverse between routes, and U > V will be tackled in a later
    // iteration.
    if (U->route() != V->route() || U->idx() + 1 >= V->idx())
        return 0;

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

void ReverseSegment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->route() == V->route());
    auto *nU = n(U);

    while (V->idx() > nU->idx())
    {
        auto *pV = p(V);
        Route::swap(nU, V);
        nU = n(V);  // after swap, V is now nU
        V = pV;
    }
}
