#include "InsertOptional.h"

#include "primitives.h"

#include <cassert>

using pyvrp::search::InsertOptional;

std::pair<pyvrp::Cost, bool> InsertOptional::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    if (U->route() || !V->route())
        return std::make_pair(0, false);

    auto const deltaCost = insertCost(U, V, data, costEvaluator);
    return std::make_pair(deltaCost, deltaCost < 0);
}

void InsertOptional::apply(Route::Node *U, Route::Node *V) const
{
    assert(!U->route() && V->route());
    auto *route = V->route();
    route->insert(V->idx() + 1, U);
}

template <>
bool pyvrp::search::supports<InsertOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())   // need at least one optional
        if (!client.required && !client.group)  // client not part of a group
            return true;

    return false;
}
