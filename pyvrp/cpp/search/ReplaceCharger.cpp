#include "ReplaceCharger.h"
#include "ChargerSegment.h"
#include "InsertCharger.h"
#include "constants.h"

#include <cassert>

using pyvrp::search::ReplaceCharger;

void ReplaceCharger::evaluatePrev(Route::Node *U,
                                  CostEvaluator const &costEvaluator)
{
    auto const *route = U->route();
    auto const pos = U->pos();
    auto const duration = route->rechargeDuration();
    auto const current = p(U)->idx();

    for (size_t charger = 0; charger != data.numChargers(); ++charger)
    {
        if (charger != current)  // new charger in the same place
        {
            Cost deltaCost = 0;
            costEvaluator.deltaCost<true>(
                deltaCost,
                Route::Proposal(route->before(pos - 2),
                                ChargerSegment(data, charger, duration),
                                route->after(pos)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, charger, pos - 1, pos - 1};
        }

        if (!n(U)->isCharger())  // charger to the other side of U
        {
            Cost deltaCost = 0;
            costEvaluator.deltaCost<true>(
                deltaCost,
                Route::Proposal(route->before(pos - 2),
                                route->at(pos),
                                ChargerSegment(data, charger, duration),
                                route->after(pos + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, charger, pos - 1, pos};
        }
    }
}

void ReplaceCharger::evaluateNext(Route::Node *U,
                                  CostEvaluator const &costEvaluator)
{
    auto const *route = U->route();
    auto const pos = U->pos();
    auto const duration = route->rechargeDuration();
    auto const current = n(U)->idx();

    for (size_t charger = 0; charger != data.numChargers(); ++charger)
    {
        if (charger != current)  // new charger in the same place
        {
            Cost deltaCost = 0;
            costEvaluator.deltaCost<true>(
                deltaCost,
                Route::Proposal(route->before(pos),
                                ChargerSegment(data, charger, duration),
                                route->after(pos + 2)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, charger, pos + 1, pos + 1};
        }

        if (!p(U)->isCharger())  // charger to the other side of U
        {
            Cost deltaCost = 0;
            costEvaluator.deltaCost<true>(
                deltaCost,
                Route::Proposal(route->before(pos - 1),
                                ChargerSegment(data, charger, duration),
                                route->at(pos),
                                route->after(pos + 2)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, charger, pos + 1, pos};
        }
    }
}

std::pair<pyvrp::Cost, bool>
ReplaceCharger::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot());
    stats_.numEvaluations++;

    if (!U->route())
        return std::make_pair(0, false);

    move_ = {};

    if (p(U)->isCharger())
        evaluatePrev(U, costEvaluator);

    if (n(U)->isCharger())
        evaluateNext(U, costEvaluator);

    return std::make_pair(move_.cost, move_.cost < -TOL);
}

void ReplaceCharger::apply(Route::Node *U) const
{
    stats_.numApplications++;
    auto *route = U->route();

    Route::Node charger = {Activity::ActivityType::CHARGER, move_.charger};
    route->remove(move_.removePos);
    route->insert(move_.insertPos, &charger);
}

std::string ReplaceCharger::name() const { return "ReplaceCharger"; }

bool ReplaceCharger::supports(ProblemData const &data)
{
    // Replacing a charger requires another charger to replace it with. The
    // move to the other side of U also works with a single charger.
    return InsertCharger::supports(data);
}
