#include "RelocateStep.h"

#include <cassert>

using pyvrp::search::RelocateStep;

std::pair<pyvrp::Cost, bool>
RelocateStep::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    move_ = {};

    auto *route = U->route();
    auto *pickup = U;
    auto *delivery = U + 1;
    assert(delivery->route() == route);
    assert(pickup->pos() < delivery->pos());
    assert(pickup->trip() == delivery->trip());

    auto const evaluate = [&](Route::Node *step, Route::Node const *before)
    {
        if (before == step || p(before) == step)
            return false;

        Cost deltaCost = 0;
        if (step->pos() < before->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(step->pos() - 1),
                    route->between(step->pos() + 1, before->pos() - 1),
                    route->at(step->pos()),
                    route->after(before->pos())));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(before->pos() - 1),
                                route->at(step->pos()),
                                route->between(before->pos(), step->pos() - 1),
                                route->after(step->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {deltaCost, step, before};
            return true;
        }

        return false;
    };

    for (auto const *node = delivery; !node->isDepot(); node = p(node))
        if (evaluate(pickup, node))
            return std::make_pair(move_.cost, true);

    for (auto const *node = n(pickup);; node = n(node))
    {
        if (evaluate(delivery, node))
            break;

        if (node->isDepot())
            break;
    }

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocateStep::apply(Route::Node *U) const
{
    assert(U->isPickup() && U->route());
    assert(move_.step && move_.before);
    assert(move_.step == U || move_.step == U + 1);
    stats_.numApplications++;

    auto *route = U->route();
    route->remove(move_.step->pos());
    route->insert(move_.before->pos(), move_.step);
}

std::string RelocateStep::name() const { return "RelocateStep"; }

bool RelocateStep::supports(ProblemData const &data)
{
    return data.numShipments() > 0;
}
