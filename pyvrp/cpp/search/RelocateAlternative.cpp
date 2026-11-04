#include "RelocateAlternative.h"

#include "ClientSegment.h"
#include "DeliverySegment.h"
#include "PickupSegment.h"
#include "constants.h"

#include <cassert>

using pyvrp::search::RelocateAlternative;

pyvrp::Cost
RelocateAlternative::evalToShipment(Route::Node const *U,
                                    Route::Node const *V,
                                    size_t alternative,
                                    Route::Node const *deliveryAfter,
                                    Cost deltaCost,
                                    CostEvaluator const &costEvaluator) const
{
    if (U->isPickup() && n(U) != U + 1)
        return evalShipmentToShipment(
            U, V, alternative, deliveryAfter, deltaCost, costEvaluator);

    // A client or adjacent shipment is removed as one contiguous block.
    auto const *route = U->route();
    auto const u = U->pos();
    auto const last = U->isPickup() ? (U + 1)->pos() : u;
    auto const v = V->pos();
    auto const w = deliveryAfter->pos();
    auto const pickup = PickupSegment(data, alternative);
    auto const delivery = DeliverySegment(data, alternative);

    if (v == w)  // adjacent pickup and delivery
    {
        if (u < v)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(u - 1),
                                                    route->between(last + 1, v),
                                                    pickup,
                                                    delivery,
                                                    route->after(v + 1)));
        else
        {
            if (u == v + 1)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    delivery,
                                    route->after(last + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    delivery,
                                    route->between(v + 1, u - 1),
                                    route->after(last + 1)));
        }
    }
    else if (u < v)  // U ... V ... W
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(u - 1),
                                                route->between(last + 1, v),
                                                pickup,
                                                route->between(v + 1, w),
                                                delivery,
                                                route->after(w + 1)));
    else if (u < w)  // V ... U ... W
    {
        if (u == v + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(v),
                                                    pickup,
                                                    route->between(last + 1, w),
                                                    delivery,
                                                    route->after(w + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, u - 1),
                                route->between(last + 1, w),
                                delivery,
                                route->after(w + 1)));
    }
    else  // V ... W ... U
    {
        if (u == w + 1)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(v),
                                                    pickup,
                                                    route->between(v + 1, w),
                                                    delivery,
                                                    route->after(last + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, w),
                                delivery,
                                route->between(w + 1, u - 1),
                                route->after(last + 1)));
    }

    return deltaCost;
}

pyvrp::Cost
RelocateAlternative::evalToClient(Route::Node const *U,
                                  Route::Node const *V,
                                  size_t alternative,
                                  Cost deltaCost,
                                  CostEvaluator const &costEvaluator) const
{
    auto const *route = U->route();
    auto const p = U->pos();
    auto const d = U->isPickup() ? (U + 1)->pos() : p;
    auto const v = V->pos();
    auto const client = ClientSegment(data, alternative);

    if (U->isClient() || n(U) == U + 1)
    {
        if (U->isClient() && U == n(V))  // handled by ReplaceGroup
            return 0;

        if (d < v)
            costEvaluator.deltaCost(deltaCost,
                                    Route::Proposal(route->before(p - 1),
                                                    route->between(d + 1, v),
                                                    client,
                                                    route->after(v + 1)));
        else if (p == v + 1)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v), client, route->after(d + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                client,
                                route->between(v + 1, p - 1),
                                route->after(d + 1)));
        return deltaCost;
    }

    if (d < v)  // pickup ... delivery ... V
    {
        costEvaluator.deltaCost(deltaCost,
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
        if (p == v + 1)
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                client,
                                route->between(p + 1, d - 1),
                                route->after(d + 1)));
        }
        else
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                client,
                                route->between(v + 1, p - 1),
                                route->between(p + 1, d - 1),
                                route->after(d + 1)));
        }
    }

    return deltaCost;
}

pyvrp::Cost RelocateAlternative::evalShipmentToShipment(
    Route::Node const *U,
    Route::Node const *V,
    size_t alternative,
    Route::Node const *deliveryAfter,
    Cost deltaCost,
    CostEvaluator const &costEvaluator) const
{
    assert(U->isPickup() && n(U) != U + 1);
    auto const *route = U->route();
    auto const p = U->pos();
    auto const d = (U + 1)->pos();
    auto const v = V->pos();
    auto const w = deliveryAfter->pos();
    auto const pickup = PickupSegment(data, alternative);
    auto const delivery = DeliverySegment(data, alternative);

    // V and W anchor the new pickup and delivery; W == V makes them adjacent.
    if (d < v)  // old pickup ... old delivery ... V ... W
    {
        if (w == v)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, d - 1),
                                route->between(d + 1, v),
                                pickup,
                                delivery,
                                route->after(w + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(p - 1),
                                route->between(p + 1, d - 1),
                                route->between(d + 1, v),
                                pickup,
                                route->between(v + 1, w),
                                delivery,
                                route->after(w + 1)));
    }
    else if (p < v)
    {
        if (d < w)  // old pickup ... V ... old delivery ... W
        {
            if (d == v + 1)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(p - 1),
                                    route->between(p + 1, v),
                                    pickup,
                                    route->between(d + 1, w),
                                    delivery,
                                    route->after(w + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(p - 1),
                                    route->between(p + 1, v),
                                    pickup,
                                    route->between(v + 1, d - 1),
                                    route->between(d + 1, w),
                                    delivery,
                                    route->after(w + 1)));
        }
        else  // old pickup ... V ... W ... old delivery
        {
            if (w == v)
            {
                if (d == w + 1)
                    costEvaluator.deltaCost(
                        deltaCost,
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
                                        delivery,
                                        route->between(w + 1, d - 1),
                                        route->after(d + 1)));
            }
            else
            {
                if (d == w + 1)
                    costEvaluator.deltaCost(
                        deltaCost,
                        Route::Proposal(route->before(p - 1),
                                        route->between(p + 1, v),
                                        pickup,
                                        route->between(v + 1, w),
                                        delivery,
                                        route->after(d + 1)));
                else
                    costEvaluator.deltaCost(
                        deltaCost,
                        Route::Proposal(route->before(p - 1),
                                        route->between(p + 1, v),
                                        pickup,
                                        route->between(v + 1, w),
                                        delivery,
                                        route->between(w + 1, d - 1),
                                        route->after(d + 1)));
            }
        }
    }
    else if (d < w)  // V ... old pickup ... old delivery ... W
    {
        if (p == v + 1)
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(p + 1, d - 1),
                                route->between(d + 1, w),
                                delivery,
                                route->after(w + 1)));
        }
        else
        {
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(v),
                                pickup,
                                route->between(v + 1, p - 1),
                                route->between(p + 1, d - 1),
                                route->between(d + 1, w),
                                delivery,
                                route->after(w + 1)));
        }
    }
    else if (p < w)  // V ... old pickup ... W ... old delivery
    {
        if (p == v + 1)
        {
            if (d == w + 1)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(p + 1, w),
                                    delivery,
                                    route->after(d + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(p + 1, w),
                                    delivery,
                                    route->between(w + 1, d - 1),
                                    route->after(d + 1)));
        }
        else
        {
            if (d == w + 1)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(v + 1, p - 1),
                                    route->between(p + 1, w),
                                    delivery,
                                    route->after(d + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(v + 1, p - 1),
                                    route->between(p + 1, w),
                                    delivery,
                                    route->between(w + 1, d - 1),
                                    route->after(d + 1)));
        }
    }
    else  // V ... W ... old pickup ... old delivery
    {
        if (w == v)
        {
            if (p == w + 1)
            {
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    delivery,
                                    route->between(p + 1, d - 1),
                                    route->after(d + 1)));
            }
            else
            {
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    delivery,
                                    route->between(w + 1, p - 1),
                                    route->between(p + 1, d - 1),
                                    route->after(d + 1)));
            }
        }
        else
        {
            if (p == w + 1)
            {
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(v + 1, w),
                                    delivery,
                                    route->between(p + 1, d - 1),
                                    route->after(d + 1)));
            }
            else
            {
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(v),
                                    pickup,
                                    route->between(v + 1, w),
                                    delivery,
                                    route->between(w + 1, p - 1),
                                    route->between(p + 1, d - 1),
                                    route->after(d + 1)));
            }
        }
    }

    return deltaCost;
}

void RelocateAlternative::evalSameRoute(Route::Node *U,
                                        Route::Node *V,
                                        CostEvaluator const &costEvaluator)
{
    auto const group = U->isClient() ? data.client(U->idx()).group
                                     : data.shipment(U->idx()).group;
    auto const uPrize = U->isClient() ? data.client(U->idx()).prize
                                      : data.shipment(U->idx()).prize;

    if (U->trip() != V->trip())
        return;

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
            auto const deltaCost = evalToClient(
                U, V, alternative->idx(), uPrize - prize, costEvaluator);
            if (deltaCost < -TOL)
            {
                move_ = {deltaCost, alternative};
                return;
            }
            continue;
        }

        // Try adjacent pickup and delivery, then later delivery positions.
        auto const *delivery = U->isPickup() ? U + 1 : nullptr;
        for (auto const *after = V;; after = n(after))
        {
            if (after != U && after != delivery)
            {
                auto const deltaCost = evalToShipment(U,
                                                      V,
                                                      alternative->idx(),
                                                      after,
                                                      uPrize - prize,
                                                      costEvaluator);

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
