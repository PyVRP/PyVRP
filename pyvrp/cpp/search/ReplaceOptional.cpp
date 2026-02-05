#include "ReplaceOptional.h"

#include "ClientSegment.h"

#include <cassert>

using pyvrp::search::ReplaceOptional;

std::pair<pyvrp::Cost, bool> ReplaceOptional::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (U->route() || !V->route() || V->isDepot())
        return std::make_pair(0, false);

    ProblemData::Client const &uData = data.location(U->client());
    ProblemData::Client const &vData = data.location(V->client());
    if (vData.required || uData.group != vData.group)
        // Cannot replace required clients, or clients that are not in the
        // same mutually exclusive group.
        return std::make_pair(0, false);

    auto *route = V->route();
    Cost deltaCost = vData.prize - uData.prize;
    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(V->idx() - 1),
                                            ClientSegment(data, U->client()),
                                            route->after(V->idx() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void ReplaceOptional::apply(Route::Node *U, Route::Node *V) const
{
    assert(!U->route() && V->route());
    stats_.numApplications++;

    auto *route = V->route();
    auto const idx = V->idx();
    route->remove(idx);
    route->insert(idx, U);
}

template <>
bool pyvrp::search::supports<ReplaceOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())  // need at least one optional
        if (!client.required)                  // client
            return true;

    return false;
}
