#include "RelocateAlternative.h"

#include "ClientSegment.h"
#include "DeliverySegment.h"
#include "PickupSegment.h"
#include "constants.h"

#include <cassert>

using pyvrp::search::RelocateAlternative;

pyvrp::Cost RelocateAlternative::evalClientToClient(
    Route::Node const *U,
    Route::Node const *V,
    size_t alternative,
    Cost deltaCost,
    CostEvaluator const &costEvaluator) const
{
    assert(U->isClient());
    auto const *route = U->route();
    auto const u = U->pos();
    auto const v = V->pos();
    auto const client = ClientSegment(data, alternative);

    if (u < v)
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(u - 1),
                                                route->between(u + 1, v),
                                                client,
                                                route->after(v + 1)));
    else
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(v),
                                                client,
                                                route->between(v + 1, u - 1),
                                                route->after(u + 1)));

    return deltaCost;
}

pyvrp::Cost RelocateAlternative::evalShipmentToClient(
    Route::Node const *U,
    Route::Node const *V,
    size_t alternative,
    Cost deltaCost,
    CostEvaluator const &costEvaluator) const
{
    assert(U->isPickup());
    auto const *route = U->route();
    auto const p = U->pos();
    auto const d = (U + 1)->pos();
    auto const v = V->pos();
    auto const client = ClientSegment(data, alternative);

    if (d < v)  // pickup ... delivery ... V
    {
        if (d == p + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(p - 1),
                                                    route->between(d + 1, v),
                                                    client,
                                                    route->after(v + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, d - 1),
                                route->between(d + 1, v),
                                client,
                                route->after(v + 1)));
    }
    else if (p < v)  // pickup ... V ... delivery
    {
        if (d == v + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(p - 1),
                                                    route->between(p + 1, v),
                                                    client,
                                                    route->after(d + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, v),
                                client,
                                route->between(v + 1, d - 1),
                                route->after(d + 1)));
    }
    else  // V ... pickup ... delivery
    {
        if (d == p + 1)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                client,
                                route->between(v + 1, p - 1),
                                route->after(d + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                client,
                                route->between(v + 1, p - 1),
                                route->between(p + 1, d - 1),
                                route->after(d + 1)));
    }

    return deltaCost;
}

pyvrp::Cost RelocateAlternative::evalClientToShipment(
    Route::Node const *U,
    Route::Node const *V,
    size_t alternative,
    Route::Node const *deliveryAfter,
    Cost deltaCost,
    CostEvaluator const &costEvaluator) const
{
    assert(U->isClient());
    auto const *route = U->route();
    auto const u = U->pos();
    auto const v = V->pos();
    auto const w = deliveryAfter->pos();
    auto const pickup = PickupSegment(data, alternative);
    auto const delivery = DeliverySegment(data, alternative);

    if (v == w)  // adjacent pickup and delivery
    {
        if (u < v)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(u - 1),
                                                    route->between(u + 1, v),
                                                    pickup,
                                                    delivery,
                                                    route->after(v + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                delivery,
                                route->between(v + 1, u - 1),
                                route->after(u + 1)));
    }
    else if (u < v)  // U ... V ... W
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(u - 1),
                                                route->between(u + 1, v),
                                                pickup,
                                                route->between(v + 1, w),
                                                delivery,
                                                route->after(w + 1)));
    else if (u < w)  // V ... U ... W
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(v),
                                                pickup,
                                                route->between(v + 1, u - 1),
                                                route->between(u + 1, w),
                                                delivery,
                                                route->after(w + 1)));
    else  // V ... W ... U
    {
        if (u == w + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(v),
                                                    pickup,
                                                    route->between(v + 1, w),
                                                    delivery,
                                                    route->after(u + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, w),
                                delivery,
                                route->between(w + 1, u - 1),
                                route->after(u + 1)));
    }

    return deltaCost;
}

pyvrp::Cost RelocateAlternative::evalShipmentToShipment(
    Route::Node const *U,
    Route::Node const *V,
    size_t alternative,
    Cost deltaCost,
    CostEvaluator const &costEvaluator) const
{
    assert(U->isPickup() && U + 1 != V);
    auto const *route = U->route();
    auto const p = U->pos();
    auto const d = (U + 1)->pos();
    auto const v = V->pos();

    auto const pickup = PickupSegment(data, alternative);
    auto const delivery = DeliverySegment(data, alternative);

    if (d < v)  // relocate both endpoints directly after V
    {
        if (d == p + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(p - 1),
                                                    route->between(d + 1, v),
                                                    pickup,
                                                    delivery,
                                                    route->after(v + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, d - 1),
                                route->between(d + 1, v),
                                pickup,
                                delivery,
                                route->after(v + 1)));
    }
    else if (p < v)  // old pickup ... V ... delivery
    {
        if (d == v + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(p - 1),
                                                    route->between(p + 1, v),
                                                    pickup,
                                                    delivery,
                                                    route->after(d + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, v),
                                pickup,
                                route->between(v + 1, d - 1),
                                delivery,
                                route->after(d + 1)));
    }
    else  // V ... old pickup ... delivery
    {
        if (d == p + 1)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, p - 1),
                                delivery,
                                route->after(d + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, p - 1),
                                route->between(p + 1, d - 1),
                                delivery,
                                route->after(d + 1)));
    }

    return deltaCost;
}

void RelocateAlternative::evalSameRoute(Route::Node *U,
                                        Route::Node *V,
                                        CostEvaluator const &costEvaluator)
{
    // In-place replacements are handled by ReplaceGroup.
    if (U == n(V) || U->trip() != V->trip())
        return;

    auto const group = U->isClient() ? data.client(U->idx()).group
                                     : data.shipment(U->idx()).group;
    auto const uPrize = U->isClient() ? data.client(U->idx()).prize
                                      : data.shipment(U->idx()).prize;

    for (auto const &activity : data.group(*group))
    {
        auto *alternative = (*solution_)[activity];
        if (alternative == U)
            continue;

        auto const prize = activity.isClient()
                               ? data.client(activity.idx()).prize
                               : data.shipment(activity.idx()).prize;
        auto const prizeDelta = uPrize - prize;
        auto const idx = alternative->idx();

        if (alternative->isClient())
        {
            auto const deltaCost
                = U->isClient()
                      ? evalClientToClient(U, V, idx, prizeDelta, costEvaluator)
                      : evalShipmentToClient(
                            U, V, idx, prizeDelta, costEvaluator);
            if (deltaCost < -TOL)
            {
                move_ = {deltaCost, alternative};
                return;
            }
            continue;
        }

        if (U->isPickup())
        {
            auto const deltaCost
                = evalShipmentToShipment(U, V, idx, prizeDelta, costEvaluator);
            if (deltaCost < -TOL)
            {
                auto const *after = V;
                if (V->pos() < (U + 1)->pos())
                    after = n(U) == U + 1 ? p(U) : p(U + 1);
                move_ = {deltaCost, alternative, after};
                return;
            }
            continue;
        }

        // Replacing a client: try adjacent pickup and delivery, then later
        // delivery positions.
        for (auto const *after = V;; after = n(after))
        {
            if (after != U)
            {
                auto const deltaCost = evalClientToShipment(
                    U, V, idx, after, prizeDelta, costEvaluator);

                if (deltaCost < -TOL)
                {
                    move_ = {deltaCost, alternative, after};
                    return;
                }
            }
            if (n(after)->isDepot())
                break;
        }
    }
}

void RelocateAlternative::evalDifferentRoutes(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const group = U->isClient() ? data.client(U->idx()).group
                                     : data.shipment(U->idx()).group;
    auto const uPrize = U->isClient() ? data.client(U->idx()).prize
                                      : data.shipment(U->idx()).prize;
    auto const idx = (U->isClient() ? 0 : data.numClients()) + U->idx();

    if (!hasCachedRemoveCost_[idx])
    {
        Cost removeCost = uPrize;
        if (uRoute->numClients() + uRoute->numShipments() == 1)
            // This move leaves the route empty, so the cost delta is just the
            // current route cost.
            removeCost -= costEvaluator.penalisedCost(*uRoute);
        else if (U->isClient())
            costEvaluator.deltaCost<true>(  // exact so we get the right delta
                removeCost,                 // when inserting the alternative
                Route::Proposal(uRoute->before(U->pos() - 1),
                                uRoute->after(U->pos() + 1)));
        else if (n(U) == U + 1)
            costEvaluator.deltaCost<true>(
                removeCost,
                Route::Proposal(uRoute->before(U->pos() - 1),
                                uRoute->after((U + 1)->pos() + 1)));
        else
            costEvaluator.deltaCost<true>(
                removeCost,
                Route::Proposal(
                    uRoute->before(U->pos() - 1),
                    uRoute->between(U->pos() + 1, (U + 1)->pos() - 1),
                    uRoute->after((U + 1)->pos() + 1)));

        removeCost_[idx] = removeCost;
        hasCachedRemoveCost_[idx] = true;
    }

    for (auto const &activity : data.group(*group))
    {
        auto *alternative = (*solution_)[activity];
        if (alternative == U)
            continue;

        auto const prize = activity.isClient()
                               ? data.client(activity.idx()).prize
                               : data.shipment(activity.idx()).prize;
        if (alternative->isClient())
        {
            Cost deltaCost = removeCost_[idx] - prize;
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(vRoute->before(V->pos()),
                                ClientSegment(data, alternative->idx()),
                                vRoute->after(V->pos() + 1)));
            if (deltaCost < -TOL)
            {
                move_ = {deltaCost, alternative};
                return;
            }
            continue;
        }

        for (auto const *after = V;; after = n(after))
        {
            Cost deltaCost = removeCost_[idx] - prize;
            if (after == V)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(vRoute->before(V->pos()),
                                    PickupSegment(data, alternative->idx()),
                                    DeliverySegment(data, alternative->idx()),
                                    vRoute->after(V->pos() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(vRoute->before(V->pos()),
                                    PickupSegment(data, alternative->idx()),
                                    vRoute->between(V->pos() + 1, after->pos()),
                                    DeliverySegment(data, alternative->idx()),
                                    vRoute->after(after->pos() + 1)));

            if (deltaCost < -TOL)
            {
                move_ = {deltaCost, alternative, after};
                return;
            }

            if (n(after)->isDepot())
                break;
        }
    }
}

std::pair<pyvrp::Cost, bool> RelocateAlternative::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot() && solution_);
    stats_.numEvaluations++;

    if (U->isDelivery() || !U->route() || !V->route() || U == V
        || (U->isPickup() && U + 1 == V))
        return std::make_pair(0, false);

    auto const group = U->isClient() ? data.client(U->idx()).group
                                     : data.shipment(U->idx()).group;
    if (!group)
        return std::make_pair(0, false);

    move_ = {};

    if (U->route() == V->route())
        evalSameRoute(U, V, costEvaluator);
    else
        evalDifferentRoutes(U, V, costEvaluator);

    return std::make_pair(move_.cost, move_.cost < -TOL);
}

void RelocateAlternative::apply(Route::Node *U, Route::Node *V) const
{
    assert((U->isClient() || U->isPickup()) && U->route() && V->route());
    assert(move_.alternative && !move_.alternative->route());
    stats_.numApplications++;

    auto *uRoute = U->route();
    if (U->isPickup())
        uRoute->remove((U + 1)->pos());
    uRoute->remove(U->pos());

    if (move_.alternative->isPickup())
    {
        assert(move_.deliveryAfter);
        V->route()->insert(move_.deliveryAfter->pos() + 1,
                           move_.alternative + 1);
    }
    V->route()->insert(V->pos() + 1, move_.alternative);
}

void RelocateAlternative::init(Solution &solution)
{
    BinaryOperator::init(solution);
    solution_ = &solution;
    hasCachedRemoveCost_.reset();
}

std::string RelocateAlternative::name() const { return "RelocateAlternative"; }

bool RelocateAlternative::supports(ProblemData const &data)
{
    return data.numGroups() > 0;
}

void RelocateAlternative::update(Route const *route)
{
    for (auto const *node : *route)
        if (node->isClient() || node->isPickup())
            hasCachedRemoveCost_[(node->isClient() ? 0 : data.numClients())
                                 + node->idx()]
                = false;
}

RelocateAlternative::RelocateAlternative(ProblemData const &data)
    : BinaryOperator(data),
      hasCachedRemoveCost_(data.numClients() + data.numShipments()),
      removeCost_(data.numClients() + data.numShipments())
{
}
