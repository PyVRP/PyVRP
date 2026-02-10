#include "RemoveOptional.h"

#include <cassert>

using pyvrp::search::RemoveOptional;

std::pair<pyvrp::Cost, bool>
RemoveOptional::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(U->client());
    stats_.numEvaluations++;

    ProblemData::Client const &uData = data.location(U->client());
    if (!U->route() || uData.required)
        return std::make_pair(0, false);

    if (uData.group && data.group(*uData.group).required)  // cannot remove
        return std::make_pair(0, false);                   // required member

    auto *route = U->route();
    Cost deltaCost
        = uData.prize
          - Cost(route->numClients() == 1) * route->fixedVehicleCost();

    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(U->idx() - 1),
                                            route->after(U->idx() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void RemoveOptional::apply(Route::Node *U) const
{
    stats_.numApplications++;
    U->route()->remove(U->idx());
}

template <>
bool pyvrp::search::supports<RemoveOptional>(ProblemData const &data)
{
    for (auto const &client : data.clients())  // need at least one optional
        if (!client.required)                  // client
            return true;

    return false;
}
