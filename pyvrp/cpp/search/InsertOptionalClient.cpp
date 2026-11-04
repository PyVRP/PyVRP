#include "InsertOptionalClient.h"

#include "ClientSegment.h"
#include "constants.h"

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

        for (auto const &activity : group)
            if ((*solution_)[activity]->route())
                return std::make_pair(0,
                                      false);  // cannot insert another member
    }

    auto *route = V->route();
    Cost deltaCost = -uData.prize;

    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(V->pos()),
                                            ClientSegment(data, U->idx()),
                                            route->after(V->pos() + 1)));

    return std::make_pair(deltaCost, deltaCost < -TOL);
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
    BinaryOperator::init(solution);
    solution_ = &solution;
}

std::string InsertOptionalClient::name() const
{
    return "InsertOptionalClient";
}

bool InsertOptionalClient::supports(ProblemData const &data)
{
    for (auto const &client : data.clients())
    {
        if (!client.required && !client.group)
            return true;

        if (client.group && !data.group(*client.group).required)
            return true;
    }

    return false;
}
