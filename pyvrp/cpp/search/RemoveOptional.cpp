#include "RemoveOptional.h"

#include "primitives.h"

using pyvrp::search::RemoveOptional;

pyvrp::Cost RemoveOptional::evaluate(Route::Node *U,
                                     CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    ProblemData::Client const &uData = data.location(U->client());
    if (!U->route() || uData.required || uData.group)
        return 0;

    return removeCost(U, data, costEvaluator);
}

void RemoveOptional::apply(Route::Node *U) const
{
    stats_.numApplications++;
    U->route()->remove(U->idx());
}

template <>
bool pyvrp::search::supports<RemoveOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())   // need at least one optional
        if (!client.required && !client.group)  // client not part of a group
            return true;

    return false;
}
