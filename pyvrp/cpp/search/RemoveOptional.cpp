#include "RemoveOptional.h"

#include "primitives.h"

using pyvrp::search::RemoveOptional;

std::pair<pyvrp::Cost, bool>
RemoveOptional::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    ProblemData::Client const &uData = data.location(U->client());
    if (!U->route() || uData.required)
        return std::make_pair(0, false);

    if (uData.group)
    {
        assert(solution_);
        auto const &group = data.group(*uData.group);

        size_t count = 0;
        for (auto const client : group.clients())
            count += solution_->nodes[client].route() != nullptr;

        if (group.required && count == 1)     // then we cannot remove the only
            return std::make_pair(0, false);  // group member
    }

    auto const deltaCost = removeCost(U, data, costEvaluator);
    return std::make_pair(deltaCost, deltaCost < 0);
}

void RemoveOptional::apply(Route::Node *U) const
{
    stats_.numApplications++;
    U->route()->remove(U->idx());
}

void RemoveOptional::init(Solution const &solution)
{
    stats_ = {};
    solution_ = &solution;
}

template <>
bool pyvrp::search::supports<RemoveOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())  // need at least one optional
        if (!client.required)                  // client
            return true;

    return false;
}
