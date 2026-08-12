#include "RelocateWithDepot.h"

#include "Activity.h"
#include "DepotSegment.h"

#include <cassert>

using pyvrp::search::RelocateWithDepot;

void RelocateWithDepot::evalSameRoute(Route::Node *U,
                                      Route::Node *V,
                                      CostEvaluator const &costEvaluator)
{
    assert(U->route() == V->route());
    auto const *route = U->route();
    auto const &vehType = data.vehicleType(route->vehicleType());

    if (!V->isReloadDepot())  // depot first, U after
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = 0;
            if (U->pos() < V->pos())
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(U->pos() - 1),
                                    route->between(U->pos() + 1, V->pos()),
                                    DepotSegment(data, depot),
                                    route->at(U->pos()),
                                    route->after(V->pos() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V->pos()),
                                    DepotSegment(data, depot),
                                    route->at(U->pos()),
                                    route->between(V->pos() + 1, U->pos() - 1),
                                    route->after(U->pos() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::DEPOT_U, depot};
        }

    if (!n(V)->isReloadDepot())  // U first, depot after
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = 0;
            if (U->pos() < V->pos())
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(U->pos() - 1),
                                    route->between(U->pos() + 1, V->pos()),
                                    route->at(U->pos()),
                                    DepotSegment(data, depot),
                                    route->after(V->pos() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V->pos()),
                                    route->at(U->pos()),
                                    DepotSegment(data, depot),
                                    route->between(V->pos() + 1, U->pos() - 1),
                                    route->after(U->pos() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::U_DEPOT, depot};
        }
}

void RelocateWithDepot::evalDifferentRoutes(Route::Node *U,
                                            Route::Node *V,
                                            CostEvaluator const &costEvaluator)
{
    assert(U->route() != V->route());
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const &vehType = data.vehicleType(vRoute->vehicleType());

    if (!isCached_[U->idx()])
    {
        Cost removeCost = 0;
        if (uRoute->numClients() == 1 && uRoute->numShipments() == 0)
            // This move leaves the route empty, so the cost delta is just the
            // current route cost.
            removeCost -= costEvaluator.penalisedCost(*uRoute);
        else
            costEvaluator.deltaCost<true>(  // exact evaluation when removing U
                removeCost,                 // so we get the right delta later
                Route::Proposal(uRoute->before(U->pos() - 1),
                                uRoute->after(U->pos() + 1)));

        removeCost_[U->idx()] = removeCost;
        isCached_[U->idx()] = true;
    }

    if (!V->isReloadDepot())  // depot first, U after
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = removeCost_[U->idx()];
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(vRoute->before(V->pos()),
                                DepotSegment(data, depot),
                                uRoute->at(U->pos()),
                                vRoute->after(V->pos() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::DEPOT_U, depot};
        }

    if (!n(V)->isReloadDepot())  // U first, depot after
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = removeCost_[U->idx()];
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(vRoute->before(V->pos()),
                                uRoute->at(U->pos()),
                                DepotSegment(data, depot),
                                vRoute->after(V->pos() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::U_DEPOT, depot};
        }
}

std::pair<pyvrp::Cost, bool> RelocateWithDepot::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());
    stats_.numEvaluations++;

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (!U->isClient() || !uRoute || U == n(V) || vRoute->empty())
        return std::make_pair(0, false);

    if (vRoute->numTrips() == vRoute->maxTrips())
        return std::make_pair(0, false);

    // Cannot evaluate this move because it requires a load segment to contain
    // a reload depot in the middle, which makes concatenation far more complex.
    if (uRoute == vRoute && U->trip() != V->trip())
        return std::make_pair(0, false);

    // Cannot evaluate this move because it would insert a depot after V, but
    // there is an uncompleted shipment loaded before V that would then need to
    // visit a reload depot before delivery, which makes the load segment
    // concatenation more complex and is not handled.
    if (vRoute->numPickups(V->pos()) != vRoute->numDeliveries(V->pos()))
        return std::make_pair(0, false);

    move_ = {};

    if (uRoute == vRoute)
        evalSameRoute(U, V, costEvaluator);
    else
        evalDifferentRoutes(U, V, costEvaluator);

    return std::make_pair(move_.cost, move_.cost < 0);
}

void RelocateWithDepot::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto *uRoute = U->route();
    uRoute->remove(U->pos());

    auto *vRoute = V->route();
    Route::Node depot = {Activity::ActivityType::DEPOT, move_.depot};

    if (move_.type == MoveType::DEPOT_U)
    {
        vRoute->insert(V->pos() + 1, U);
        vRoute->insert(V->pos() + 1, &depot);
    }

    // We need to be careful to insert the depot last, because doing so could
    // invalidate V (it might trigger an update to the route's internal data
    // layout, which could invalidate V if V is a depot).
    if (move_.type == MoveType::U_DEPOT)
    {
        vRoute->insert(V->pos() + 1, U);
        vRoute->insert(V->pos() + 2, &depot);
    }
}

void RelocateWithDepot::init(Solution &solution)
{
    BinaryOperator::init(solution);
    isCached_.reset();
}

std::string RelocateWithDepot::name() const { return "RelocateWithDepot"; }

bool RelocateWithDepot::supports(ProblemData const &data)
{
    // We need at least one vehicle type for which reloading is enabled.
    for (auto const &vehType : data.vehicleTypes())
        if (!vehType.reloadDepots.empty() && vehType.maxReloads != 0)
            return true;

    return false;
}

void RelocateWithDepot::update(Route const *route)
{
    for (auto const *node : *route)
        if (node->isClient())
            isCached_[node->idx()] = false;
}

RelocateWithDepot::RelocateWithDepot(ProblemData const &data)
    : BinaryOperator(data),
      isCached_(data.numClients()),
      removeCost_(data.numClients())
{
}
