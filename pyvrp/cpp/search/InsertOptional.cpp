#include "InsertOptional.h"

#include "primitives.h"

#include <cassert>

using pyvrp::search::InsertOptional;

std::pair<pyvrp::Cost, bool> InsertOptional::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    ProblemData::Client const &uData = data.location(U->client());
    if (U->route() || !V->route())
        return std::make_pair(0, false);

    if (uData.group)
    {
        assert(solution_);
        auto const &group = data.group(*uData.group);
        if (group.required)  // then we have already inserted a client in the LS
            return std::make_pair(0, false);

        for (auto const client : group.clients())  // if any client is already
            if (solution_->nodes[client].route())  // in solution we cannot
                return std::make_pair(0, false);   // insert another
    }

    auto const deltaCost = insertCost(U, V, data, costEvaluator);
    return std::make_pair(deltaCost, deltaCost < 0);
}

void InsertOptional::apply(Route::Node *U, Route::Node *V) const
{
    assert(!U->route() && V->route());
    stats_.numApplications++;

    auto *route = V->route();
    route->insert(V->idx() + 1, U);
}

void InsertOptional::init(Solution const &solution)
{
    stats_ = {};
    solution_ = &solution;
}

template <>
bool pyvrp::search::supports<InsertOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())  // need at least one optional
        if (!client.required)                  // client
            return true;

    return false;
}
