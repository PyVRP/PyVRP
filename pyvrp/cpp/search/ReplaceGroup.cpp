#include "ReplaceGroup.h"

#include "ClientSegment.h"

#include <cassert>
#include <limits>

using pyvrp::search::ReplaceGroup;

std::pair<pyvrp::Cost, bool>
ReplaceGroup::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && solution_);
    stats_.numEvaluations++;

    if (!U->isClient() || U->route())
        return std::make_pair(0, false);

    auto const &uData = data.client(U->idx());
    if (!uData.group)
        return std::make_pair(0, false);

    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    Cost deltaCost = 0;
    for (auto const client : group)
        if (solution_->clients[client].route())
        {
            assert(client != U->idx());
            V_ = &solution_->clients[client];
            auto *route = V_->route();

            deltaCost = data.client(client).prize - uData.prize;
            costEvaluator.deltaCost(  // evaluate replacing V with U
                deltaCost,
                Route::Proposal(route->before(V_->pos() - 1),
                                ClientSegment(data, U->idx()),
                                route->after(V_->pos() + 1)));

            // At most one group member is in the solution at any one time, so
            // we can break now.
            break;
        }

    return std::make_pair(deltaCost, deltaCost < 0);
}

void ReplaceGroup::apply(Route::Node *U) const
{
    assert(U->isClient() && !U->route() && V_);
    stats_.numApplications++;

    auto *route = V_->route();
    auto const idx = V_->pos();
    route->remove(idx);
    route->insert(idx, U);
}

void ReplaceGroup::init(Solution &solution)
{
    stats_ = {};
    solution_ = &solution;
}

std::string ReplaceGroup::name() const { return "ReplaceGroup"; }

bool ReplaceGroup::supports(ProblemData const &data)
{
    for (auto const &group : data.groups())
        if (group.mutuallyExclusive)
            return true;

    return false;
}
