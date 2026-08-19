#include "RelocateShipment.h"

#include <cassert>

using pyvrp::search::RelocateShipment;

std::pair<pyvrp::Cost, bool> RelocateShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(V->route());
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route())
        return std::make_pair(0, false);

    if (U->route() == V->route())
        // It is not technically impossible to relocate within the same route,
        // but evaluating every possible route configuration requires dozens of
        // branches and proposals, which is prohibitive to fully list here.
        return std::make_pair(0, false);

    move_ = {};

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    auto const *uPickup = U;
    auto const *uDelivery = U + 1;

    if (!hasCachedRemoveCost_[uPickup->idx()])
    {
        Cost removeCost = 0;
        if (uRoute->numClients() == 0 && uRoute->numShipments() == 1)
            // This move leaves the route empty, so the cost delta is just the
            // current route cost.
            removeCost -= costEvaluator.penalisedCost(*uRoute);
        else if (n(uPickup) != uDelivery)   // exact when removing U so we have
            costEvaluator.deltaCost<true>(  // the correct delta cost for V
                removeCost,
                Route::Proposal(
                    uRoute->before(uPickup->pos() - 1),
                    uRoute->between(uPickup->pos() + 1, uDelivery->pos() - 1),
                    uRoute->after(uDelivery->pos() + 1)));
        else
            costEvaluator.deltaCost<true>(
                removeCost,
                Route::Proposal(uRoute->before(uPickup->pos() - 1),
                                uRoute->after(uDelivery->pos() + 1)));

        removeCost_[uPickup->idx()] = removeCost;
        hasCachedRemoveCost_[uPickup->idx()] = true;
    }

    Cost deltaCost = removeCost_[uPickup->idx()];
    costEvaluator.deltaCost(deltaCost,  // delivery directly after pickup
                            Route::Proposal(vRoute->before(V->pos()),
                                            uRoute->at(uPickup->pos()),
                                            uRoute->at(uDelivery->pos()),
                                            vRoute->after(V->pos() + 1)));

    if (deltaCost < 0)
    {
        move_ = {V->pos() + 1};
        return std::make_pair(deltaCost, true);
    }

    // Pickup after V, delivery later in the route.
    for (auto const *node = n(V); !node->isDepot(); node = n(node))
    {
        Cost deltaCost = removeCost_[uPickup->idx()];
        costEvaluator.deltaCost(
            deltaCost,
            Route::Proposal(vRoute->before(V->pos()),
                            uRoute->at(uPickup->pos()),
                            vRoute->between(V->pos() + 1, node->pos()),
                            uRoute->at(uDelivery->pos()),
                            vRoute->after(node->pos() + 1)));

        if (deltaCost < 0)
        {
            move_ = {node->pos() + 1};  // after node
            return std::make_pair(deltaCost, true);
        }
    }

    return std::make_pair(0, false);
}

void RelocateShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isPickup() && U->route() && V->route());
    stats_.numApplications++;

    auto *uRoute = U->route();
    uRoute->remove((U + 1)->pos());  // remove delivery
    uRoute->remove(U->pos());        // remove pickup

    auto *vRoute = V->route();
    assert(move_.pos > V->pos());
    vRoute->insert(move_.pos, U + 1);  // insert delivery
    vRoute->insert(V->pos() + 1, U);   // insert pickup
}

void RelocateShipment::init(Solution &solution)
{
    BinaryOperator::init(solution);
    hasCachedRemoveCost_.reset();
}

std::string RelocateShipment::name() const { return "RelocateShipment"; }

bool RelocateShipment::supports(ProblemData const &data)
{
    // Evaluates relocating shipments between routes, so we need at least one
    // shipment and more than one vehicle.
    return data.numShipments() > 0 && data.numVehicles() > 1;
}

void RelocateShipment::update(Route const *route)
{
    for (auto const *node : *route)
        if (node->isPickup())
            hasCachedRemoveCost_[node->idx()] = false;
}

RelocateShipment::RelocateShipment(ProblemData const &data)
    : BinaryOperator(data),
      hasCachedRemoveCost_(data.numShipments()),
      removeCost_(data.numShipments())
{
}
