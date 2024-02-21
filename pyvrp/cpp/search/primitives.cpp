#include "primitives.h"
#include "LoadSegment.h"
#include "TimeWindowSegment.h"

#include <cassert>

using pyvrp::Cost;
using TWS = pyvrp::TimeWindowSegment;

Cost pyvrp::search::insertCost(Route::Node *U,
                               Route::Node *V,
                               ProblemData const &data,
                               CostEvaluator const &costEvaluator)
{
    if (!V->route() || U->isDepot())
        return 0;

    auto *route = V->route();
    ProblemData::Client const &client = data.location(U->client());

    Distance const deltaDist = data.dist(V->client(), U->client())
                               + data.dist(U->client(), n(V)->client())
                               - data.dist(V->client(), n(V)->client());

    Cost deltaCost = static_cast<Cost>(deltaDist) - client.prize;

    deltaCost += Cost(route->empty()) * route->fixedVehicleCost();

    auto const ls = LoadSegment::merge(route->before(V->idx()),
                                       LoadSegment(client),
                                       route->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    auto const tws = TWS::merge(data.durationMatrix(),
                                route->before(V->idx()),
                                TWS(U->client(), client),
                                route->after(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(tws.timeWarp(route->maxDuration()));
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

    Distance const deltaDist = data.dist(p(U)->client(), n(U)->client())
                               - data.dist(p(U)->client(), U->client())
                               - data.dist(U->client(), n(U)->client());

    Cost deltaCost = static_cast<Cost>(deltaDist) + client.prize;

    deltaCost -= Cost(route->size() == 1) * route->fixedVehicleCost();

    auto const ls = LoadSegment::merge(route->before(U->idx() - 1),
                                       route->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    auto const tws = TWS::merge(data.durationMatrix(),
                                route->before(U->idx() - 1),
                                route->after(U->idx() + 1));

    deltaCost += costEvaluator.twPenalty(tws.timeWarp(route->maxDuration()));
    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    return deltaCost;
}
