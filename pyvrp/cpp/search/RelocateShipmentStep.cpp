#include "RelocateShipmentStep.h"

#include <cassert>

using pyvrp::search::RelocateShipmentStep;

std::pair<pyvrp::Cost, bool>
RelocateShipmentStep::evaluate(Route::Node *U,
                               CostEvaluator const &costEvaluator)
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

    for (auto const *before = delivery; !before->isDepot(); before = p(before))
    {
        if (before == pickup || p(before) == pickup)
            continue;

        Cost deltaCost = 0;
        if (pickup->pos() < before->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(pickup->pos() - 1),
                    route->between(pickup->pos() + 1, before->pos() - 1),
                    route->at(pickup->pos()),
                    route->after(before->pos())));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(before->pos() - 1),
                    route->at(pickup->pos()),
                    route->between(before->pos(), pickup->pos() - 1),
                    route->after(pickup->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {deltaCost, pickup, before};
            return std::make_pair(deltaCost, true);
        }
    }

    for (auto const *before = n(pickup);; before = n(before))
    {
        if (before == delivery || p(before) == delivery)
        {
            if (before->isDepot())
                break;

            continue;
        }

        Cost deltaCost = 0;
        if (delivery->pos() < before->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(delivery->pos() - 1),
                    route->between(delivery->pos() + 1, before->pos() - 1),
                    route->at(delivery->pos()),
                    route->after(before->pos())));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(before->pos() - 1),
                    route->at(delivery->pos()),
                    route->between(before->pos(), delivery->pos() - 1),
                    route->after(delivery->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {deltaCost, delivery, before};
            return std::make_pair(deltaCost, true);
        }

        if (before->isDepot())
            break;
    }

    return std::make_pair(move_.cost, false);
}

void RelocateShipmentStep::apply(Route::Node *U) const
{
    assert(U->isPickup() && U->route());
    assert(move_.step && move_.before);
    assert(move_.step == U || move_.step == U + 1);
    stats_.numApplications++;

    auto *route = U->route();
    route->remove(move_.step->pos());
    route->insert(move_.before->pos(), move_.step);
}

std::string RelocateShipmentStep::name() const
{
    return "RelocateShipmentStep";
}

bool RelocateShipmentStep::supports(ProblemData const &data)
{
    return data.numShipments() > 0;
}
