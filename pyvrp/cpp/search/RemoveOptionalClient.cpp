#include "RemoveOptionalClient.h"

#include <cassert>

using pyvrp::search::RemoveOptionalClient;

std::pair<pyvrp::Cost, bool>
RemoveOptionalClient::evaluate(Route::Node *U,
                               CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    auto const *route = U->route();
    if (!U->isClient() || !route)
        return std::make_pair(0, false);

    auto const &client = data.client(U->idx());
    if (client.required)
        return std::make_pair(0, false);

    if (client.group && data.group(*client.group).required)  // cannot remove
        return std::make_pair(0, false);                     // required member

    Cost deltaCost = client.prize;
    if (route->numShipments() == 0 && route->numClients() == 1)
        // This move leaves the route empty, so the cost delta is just the
        // current route cost.
        deltaCost -= costEvaluator.penalisedCost(*route);
    else
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(U->pos() - 1),
                                                route->after(U->pos() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void RemoveOptionalClient::apply(Route::Node *U) const
{
    assert(U->isClient() && U->route());
    stats_.numApplications++;
    U->route()->remove(U->pos());
}

std::string RemoveOptionalClient::name() const
{
    return "RemoveOptionalClient";
}

bool RemoveOptionalClient::supports(ProblemData const &data)
{
    for (auto const &group : data.groups())  // if the group is not required
        if (!group.required)                 // its clients are not either
            return true;

    for (auto const &client : data.clients())   // or need at least one optional
        if (!client.required && !client.group)  // client not in a group
            return true;

    return false;
}
