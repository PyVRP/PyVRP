#include "RelocateAlternative.h"

#include "ClientSegment.h"

#include <cassert>

using pyvrp::search::RelocateAlternative;

void RelocateAlternative::evalWithinRoute(Route::Node *U,
                                          Route::Node *V,
                                          CostEvaluator const &costEvaluator)
{

    auto const *route = U->route();
    auto const &uData = data.client(U->idx());
    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    if (U->trip() != V->trip())
        return;

    if (U->pos() < V->pos())
        for (auto const client : group)
        {
            auto *alternative = &solution_->clients[client];
            if (alternative == U)
                continue;

            auto const &alternativeData = data.client(client);
            Cost deltaCost = uData.prize - alternativeData.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(U->pos() - 1),
                                route->between(U->pos() + 1, V->pos()),
                                ClientSegment(data, client),
                                route->after(V->pos() + 1)));

            if (deltaCost < 0)
            {
                move_ = {deltaCost, alternative};
                return;
            }
        }
    else
        for (auto const client : group)
        {
            auto *alternative = &solution_->clients[client];
            if (alternative == U)
                continue;

            auto const &alternativeData = data.client(client);
            Cost deltaCost = uData.prize - alternativeData.prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(V->pos()),
                                ClientSegment(data, client),
                                route->between(V->pos() + 1, U->pos() - 1),
                                route->after(U->pos() + 1)));

            if (deltaCost < 0)
            {
                move_ = {deltaCost, alternative};
                return;
            }
        }
}

void RelocateAlternative::evalBetweenRoutes(Route::Node *U,
                                            Route::Node *V,
                                            CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const &uData = data.client(U->idx());
    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    Cost removeCost = uData.prize;
    if (uRoute->numClients() == 1 && uRoute->numShipments() == 0)
        // This move leaves the route empty, so the cost delta is just the
        // current route cost.
        removeCost -= costEvaluator.penalisedCost(*uRoute);
    else
        costEvaluator.deltaCost<true>(  // exact evaluation so we get the right
            removeCost,                 // delta when inserting the alternative
            Route::Proposal(uRoute->before(U->pos() - 1),
                            uRoute->after(U->pos() + 1)));

    for (auto const client : group)
    {
        if (client == U->idx())
            continue;

        auto const &alternative = data.client(client);
        Cost deltaCost = removeCost - alternative.prize;
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(vRoute->before(V->pos()),
                                                ClientSegment(data, client),
                                                vRoute->after(V->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {deltaCost, &solution_->clients[client]};
            return;
        }
    }
}

std::pair<pyvrp::Cost, bool> RelocateAlternative::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot() && solution_);
    stats_.numEvaluations++;

    if (!U->isClient() || !U->route() || !V->route() || U == V || U == n(V))
        return std::make_pair(0, false);

    auto const &client = data.client(U->idx());
    if (!client.group)
        return std::make_pair(0, false);

    move_ = {};

    if (U->route() == V->route())
        evalWithinRoute(U, V, costEvaluator);
    else
        evalBetweenRoutes(U, V, costEvaluator);

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocateAlternative::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isClient() && U->route() && V->route());
    assert(move_.alternative && !move_.alternative->route());
    stats_.numApplications++;

    U->route()->remove(U->pos());
    V->route()->insert(V->pos() + 1, move_.alternative);
}

void RelocateAlternative::init(Solution &solution)
{
    stats_ = {};
    solution_ = &solution;
}

std::string RelocateAlternative::name() const { return "RelocateAlternative"; }

bool RelocateAlternative::supports(ProblemData const &data)
{
    for (auto const &group : data.groups())
        if (group.mutuallyExclusive)
            return true;

    return false;
}
