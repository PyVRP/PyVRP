#include "ReplaceOptional.h"

#include "primitives.h"

#include <cassert>

using pyvrp::search::ReplaceOptional;

std::pair<pyvrp::Cost, bool> ReplaceOptional::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (U->route() || !V->route())
        return std::make_pair(0, false);

    ProblemData::Client const &uData = data.location(U->client());
    ProblemData::Client const &vData = data.location(V->client());
    if (vData.required || uData.group || vData.group)
        return std::make_pair(0, false);

    auto const deltaCost = inplaceCost(U, V, data, costEvaluator);
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
    for (auto const &client : data.clients())   // need at least one optional
        if (!client.required && !client.group)  // client not part of a group
            return true;

    return false;
}
