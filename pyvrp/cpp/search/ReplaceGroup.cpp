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

    ProblemData::Client const &uData = data.location(U->client());
    if (U->route() || !uData.group)
        return std::make_pair(0, false);

    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    Cost deltaCost = 0;
    for (auto const client : group)
        if (solution_->nodes[client].route())
        {
            assert(client != U->client());
            V_ = &solution_->nodes[client];
            auto *route = V_->route();

            costEvaluator.deltaCost(  // evaluate replacing V with U
                deltaCost,
                Route::Proposal(route->before(V_->idx() - 1),
                                ClientSegment(data, U->client()),
                                route->after(V_->idx() + 1)));

            // At most one group member is in the solution at any one time, so
            // we can break now.
            break;
        }

    return std::make_pair(deltaCost, deltaCost < 0);
}

void ReplaceGroup::apply(Route::Node *U) const
{
    assert(!U->route() && V_);
    stats_.numApplications++;

    auto *route = V_->route();
    auto const idx = V_->idx();
    route->remove(idx);
    route->insert(idx, U);
}

void ReplaceGroup::init(Solution &solution)
{
    stats_ = {};
    solution_ = &solution;
}

template <> bool pyvrp::search::supports<ReplaceGroup>(ProblemData const &data)
{
    return data.numGroups() > 0;
}
