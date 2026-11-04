#include "ReplaceGroup.h"

#include "ClientSegment.h"
#include "DeliverySegment.h"
#include "PickupSegment.h"
#include "constants.h"

#include <cassert>

using pyvrp::search::ReplaceGroup;

std::pair<pyvrp::Cost, bool>
ReplaceGroup::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && solution_);
    stats_.numEvaluations++;

    if (U->route() || (!U->isClient() && !U->isPickup()))
        return std::make_pair(0, false);

    auto const group = U->isClient() ? data.client(U->idx()).group
                                     : data.shipment(U->idx()).group;
    if (!group)
        return std::make_pair(0, false);

    V_ = nullptr;
    for (auto const &activity : data.group(*group))
        if (auto *node = (*solution_)[activity]; node->route())
        {
            V_ = node;
            break;
        }

    if (!V_)
        return std::make_pair(0, false);

    auto const *route = V_->route();
    auto const uPrize = U->isClient() ? data.client(U->idx()).prize
                                      : data.shipment(U->idx()).prize;
    auto const vPrize = V_->isClient() ? data.client(V_->idx()).prize
                                       : data.shipment(V_->idx()).prize;
    auto const prizeDelta = vPrize - uPrize;

    if (U->isClient() && V_->isClient())
    {
        Cost deltaCost = prizeDelta;
        costEvaluator.deltaCost(deltaCost,
                                Route::Proposal(route->before(V_->pos() - 1),
                                                ClientSegment(data, U->idx()),
                                                route->after(V_->pos() + 1)));
        return std::make_pair(deltaCost, deltaCost < -TOL);
    }

    if (V_->isClient())  // replace client with pickup, insert delivery later
    {
        for (auto const *node = V_; !node->isDepot(); node = n(node))
        {
            Cost deltaCost = prizeDelta;
            if (node == V_)
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V_->pos() - 1),
                                    PickupSegment(data, U->idx()),
                                    DeliverySegment(data, U->idx()),
                                    route->after(V_->pos() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V_->pos() - 1),
                                    PickupSegment(data, U->idx()),
                                    route->between(V_->pos() + 1, node->pos()),
                                    DeliverySegment(data, U->idx()),
                                    route->after(node->pos() + 1)));
            if (deltaCost < -TOL)
            {
                deliveryPos_ = node->pos() + 1;
                return std::make_pair(deltaCost, true);
            }
        }

        return std::make_pair(0, false);
    }

    auto *pickup = V_;
    auto *delivery = V_ + 1;
    assert(pickup->isPickup() && delivery->isDelivery());

    if (U->isPickup())  // replace both shipment nodes in place
    {
        Cost deltaCost = prizeDelta;
        if (n(pickup) == delivery)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(pickup->pos() - 1),
                                PickupSegment(data, U->idx()),
                                DeliverySegment(data, U->idx()),
                                route->after(delivery->pos() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(pickup->pos() - 1),
                    PickupSegment(data, U->idx()),
                    route->between(pickup->pos() + 1, delivery->pos() - 1),
                    DeliverySegment(data, U->idx()),
                    route->after(delivery->pos() + 1)));
        return std::make_pair(deltaCost, deltaCost < -TOL);
    }

    // Replace the shipment with a client at either shipment node's position.
    for (auto *node : {pickup, delivery})
    {
        Cost deltaCost = prizeDelta;
        if (n(pickup) == delivery)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(pickup->pos() - 1),
                                ClientSegment(data, U->idx()),
                                route->after(delivery->pos() + 1)));
        else if (node == pickup)
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(pickup->pos() - 1),
                    ClientSegment(data, U->idx()),
                    route->between(pickup->pos() + 1, delivery->pos() - 1),
                    route->after(delivery->pos() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(
                    route->before(pickup->pos() - 1),
                    route->between(pickup->pos() + 1, delivery->pos() - 1),
                    ClientSegment(data, U->idx()),
                    route->after(delivery->pos() + 1)));

        if (deltaCost < -TOL)
        {
            V_ = node;
            return std::make_pair(deltaCost, true);
        }
    }

    return std::make_pair(0, false);
}

void ReplaceGroup::apply(Route::Node *U) const
{
    assert((U->isClient() || U->isPickup()) && !U->route() && V_);
    stats_.numApplications++;

    auto *route = V_->route();
    Route::swap(U, V_);

    if (U->isPickup())
    {
        if (V_->isPickup())
            Route::swap(U + 1, V_ + 1);
        else
            route->insert(deliveryPos_, U + 1);
    }
    else if (V_->isShipment())
        route->remove((V_->isPickup() ? V_ + 1 : V_ - 1)->pos());
}

void ReplaceGroup::init(Solution &solution)
{
    UnaryOperator::init(solution);
    solution_ = &solution;
}

std::string ReplaceGroup::name() const { return "ReplaceGroup"; }

bool ReplaceGroup::supports(ProblemData const &data)
{
    return data.numGroups() > 0;
}
