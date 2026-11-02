#include "RemoveCharger.h"
#include "InsertCharger.h"
#include "constants.h"

#include <cassert>
#include <limits>

using pyvrp::search::RemoveCharger;

std::pair<pyvrp::Cost, bool>
RemoveCharger::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot());
    stats_.numEvaluations++;

    if (!U->route())
        return std::make_pair(0, false);

    auto const *route = U->route();
    Cost bestCost = std::numeric_limits<Cost>::max();

    if (p(U)->isCharger())
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost<true>(
            deltaCost,
            Route::Proposal(route->before(U->pos() - 2),
                            route->after(U->pos())));

        if (deltaCost < bestCost)
        {
            bestCost = deltaCost;
            move_ = MoveType::REMOVE_PREV;
        }
    }

    if (n(U)->isCharger())
    {
        Cost deltaCost = 0;
        costEvaluator.deltaCost<true>(
            deltaCost,
            Route::Proposal(route->before(U->pos()),
                            route->after(U->pos() + 2)));

        if (deltaCost < bestCost)
        {
            bestCost = deltaCost;
            move_ = MoveType::REMOVE_NEXT;
        }
    }

    // Apply this move if it's either better or neutral. It can be neutral if
    // e.g. the route does not need this charger and the charger is on the way.
    // We are a bit stricter here with the tolerance than when inserting a
    // charger to avoid cycling between charger insertion and removal.
    return std::make_pair(bestCost, bestCost <= TOL / 2);
}

void RemoveCharger::apply(Route::Node *U) const
{
    stats_.numApplications++;
    auto *route = U->route();

    if (move_ == MoveType::REMOVE_PREV)
        route->remove(U->pos() - 1);

    if (move_ == MoveType::REMOVE_NEXT)
        route->remove(U->pos() + 1);
}

std::string RemoveCharger::name() const { return "RemoveCharger"; }

bool RemoveCharger::supports(ProblemData const &data)
{
    return InsertCharger::supports(data);
}
