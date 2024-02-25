#include "primitives.h"

#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

#include <cassert>

using pyvrp::Cost;

Cost pyvrp::search::insertCost(Route::Node *U,
                               Route::Node *V,
                               ProblemData const &data,
                               CostEvaluator const &costEvaluator)
{
    if (!V->route() || U->isDepot())
        return 0;

    auto *route = V->route();
    ProblemData::Client const &client = data.location(U->client());

    Cost deltaCost
        = Cost(route->empty()) * route->fixedVehicleCost() - client.prize;

    auto const distSegment
        = DistanceSegment::merge(data.distanceMatrix(),
                                 route->before(V->idx()),
                                 DistanceSegment(U->client()),
                                 route->after(V->idx() + 1));

    deltaCost += static_cast<Cost>(distSegment.distance());
    deltaCost -= static_cast<Cost>(route->distance());

    auto const ls = LoadSegment::merge(route->before(V->idx()),
                                       LoadSegment(client),
                                       route->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    auto const ds = DurationSegment::merge(data.durationMatrix(),
                                           route->before(V->idx()),
                                           DurationSegment(U->client(), client),
                                           route->after(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(ds.timeWarp(route->maxDuration()));
    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    return deltaCost;
}

Cost pyvrp::search::removeCost(Route::Node *U,
                               ProblemData const &data,
                               CostEvaluator const &costEvaluator)
{
    if (!U->route() || U->isDepot())
        return 0;

    auto *route = U->route();
    ProblemData::Client const &client = data.location(U->client());

    Cost deltaCost
        = client.prize - Cost(route->size() == 1) * route->fixedVehicleCost();

    auto const distSegment = DistanceSegment::merge(data.distanceMatrix(),
                                                    route->before(U->idx() - 1),
                                                    route->after(U->idx() + 1));

    deltaCost += static_cast<Cost>(distSegment.distance());
    deltaCost -= static_cast<Cost>(route->distance());

    auto const ls = LoadSegment::merge(route->before(U->idx() - 1),
                                       route->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    auto const ds = DurationSegment::merge(data.durationMatrix(),
                                           route->before(U->idx() - 1),
                                           route->after(U->idx() + 1));

    deltaCost += costEvaluator.twPenalty(ds.timeWarp(route->maxDuration()));
    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    return deltaCost;
}
