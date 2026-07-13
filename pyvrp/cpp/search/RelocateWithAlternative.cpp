#include "RelocateWithAlternative.h"

#include "ClientSegment.h"

#include <cassert>
#include <limits>

using pyvrp::search::RelocateWithAlternative;

std::pair<pyvrp::Cost, bool> RelocateWithAlternative::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot() && solution_);
    stats_.numEvaluations++;
    alternative_ = nullptr;

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const &uData = data.client(U->idx());
    if (!uRoute || !vRoute || !uData.group || U == V || U == n(V))
        return std::make_pair(0, false);

    if (uRoute == vRoute && U->trip() != V->trip())
        return std::make_pair(0, false);

    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    Cost bestCost = std::numeric_limits<Cost>::max();
    for (auto const client : group)
    {
        auto *alternative = &solution_->nodes[client];
        if (alternative == U)
            continue;

        auto const &alternativeData = data.client(client);
        Cost deltaCost = uData.prize - alternativeData.prize;

        if (uRoute != vRoute)
        {
            if (uRoute->numClients() == 1)
                deltaCost -= uRoute->fixedVehicleCost();

            if (vRoute->empty())
                deltaCost += vRoute->fixedVehicleCost();

            auto const uProposal = Route::Proposal(uRoute->before(U->pos() - 1),
                                                   uRoute->after(U->pos() + 1));
            auto const vProposal = Route::Proposal(vRoute->before(V->pos()),
                                                   ClientSegment(data, client),
                                                   vRoute->after(V->pos() + 1));
            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
        else if (U->pos() < V->pos())
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(uRoute->before(U->pos() - 1),
                                uRoute->between(U->pos() + 1, V->pos()),
                                ClientSegment(data, client),
                                uRoute->after(V->pos() + 1)));
        }
        else
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(uRoute->before(V->pos()),
                                ClientSegment(data, client),
                                uRoute->between(V->pos() + 1, U->pos() - 1),
                                uRoute->after(U->pos() + 1)));
        }

        if (deltaCost < bestCost)
        {
            bestCost = deltaCost;
            alternative_ = alternative;
        }
    }

    return std::make_pair(bestCost, alternative_ && bestCost < 0);
}

void RelocateWithAlternative::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->route() && V->route() && alternative_ && !alternative_->route());
    stats_.numApplications++;

    U->route()->remove(U->pos());
    V->route()->insert(V->pos() + 1, alternative_);
}

void RelocateWithAlternative::init(Solution &solution)
{
    stats_ = {};
    solution_ = &solution;
}

std::string RelocateWithAlternative::name() const
{
    return "RelocateWithAlternative";
}

template <>
bool pyvrp::search::supports<RelocateWithAlternative>(ProblemData const &data)
{
    return data.numGroups() > 0;
}
