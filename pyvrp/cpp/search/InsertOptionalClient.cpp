#include "InsertOptionalClient.h"

#include "ClientSegment.h"

#include <cassert>

using pyvrp::search::InsertOptionalClient;

std::pair<pyvrp::Cost, bool> InsertOptionalClient::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->isClient() || U->route() || !V->route())
        return std::make_pair(0, false);

    auto const &uData = data.client(U->idx());
    if (uData.group)
    {
        assert(solution_);
        auto const &group = data.group(*uData.group);
        if (group.required)  // then we have already inserted a client in the LS
            return std::make_pair(0, false);

        for (auto const client : group)              // if any client is already
            if (solution_->clients[client].route())  // in solution we cannot
                return std::make_pair(0, false);     // insert another
    }

    auto *route = V->route();
    Cost deltaCost
        = Cost(route->empty()) * route->fixedVehicleCost() - uData.prize;

    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(V->pos()),
                                            ClientSegment(data, U->idx()),
                                            route->after(V->pos() + 1)));

    return std::make_pair(deltaCost, deltaCost < 0);
}

void InsertOptionalClient::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isClient() && !U->route() && V->route());
    stats_.numApplications++;

    auto *route = V->route();
    route->insert(V->pos() + 1, U);
}

void InsertOptionalClient::init(Solution &solution)
{
    stats_ = {};
    solution_ = &solution;
}

std::string InsertOptionalClient::name() const
{
    return "InsertOptionalClient";
}

template <>
bool pyvrp::search::supports<InsertOptionalClient>(ProblemData const &data)
{
    for (auto const &group : data.groups())  // if the group is not required
        if (!group.required)                 // its clients are not either
            return true;

    for (auto const &client : data.clients())   // or need at least one optional
        if (!client.required && !client.group)  // client not in a group
            return true;

    return false;
}
