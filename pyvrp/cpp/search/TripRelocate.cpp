#include "TripRelocate.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::TripRelocate;

namespace
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single reload depot.
 */
class ReloadDepotSegment
{
    pyvrp::ProblemData const &data_;
    size_t depot_;

public:
    ReloadDepotSegment(pyvrp::ProblemData const &data, size_t depot)
        : data_(data), depot_(depot)
    {
        assert(depot < data.numDepots());  // must be an actual depot
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    size_t first() const { return depot_; }
    size_t last() const { return depot_; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        pyvrp::ProblemData::Depot const &depot = data_.location(depot_);
        return {depot};
    }

    pyvrp::LoadSegment load([[maybe_unused]] size_t dimension) const
    {
        return {};
    }
};
}  // namespace

void TripRelocate::evalDepotBefore(Cost fixedCost,
                                   Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const &vehType = data.vehicleType(vRoute->vehicleType());

    if (uRoute != vRoute)
    {
        auto const uProposal = Route::Proposal(uRoute->before(U->idx() - 1),
                                               uRoute->after(U->idx() + 1));

        for (auto const depot : vehType.reloadDepots)
        {
            auto deltaCost = fixedCost;
            costEvaluator.deltaCost(
                deltaCost,
                uProposal,
                Route::Proposal(vRoute->before(V->idx()),
                                ReloadDepotSegment(data, depot),
                                uRoute->at(U->idx()),
                                vRoute->after(V->idx() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::DEPOT_U, depot};
        }
    }
    else  // within same route
    {
        auto const *route = vRoute;
        for (auto const depot : vehType.reloadDepots)
        {
            auto deltaCost = fixedCost;
            if (U->idx() < V->idx())
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(U->idx() - 1),
                                    route->between(U->idx() + 1, V->idx()),
                                    ReloadDepotSegment(data, depot),
                                    route->at(U->idx()),
                                    route->after(V->idx() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V->idx()),
                                    ReloadDepotSegment(data, depot),
                                    route->at(U->idx()),
                                    route->between(V->idx() + 1, U->idx() - 1),
                                    route->after(U->idx() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::DEPOT_U, depot};
        }
    }
}

void TripRelocate::evalDepotAfter(Cost fixedCost,
                                  Route::Node *U,
                                  Route::Node *V,
                                  CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    auto const &vehType = data.vehicleType(vRoute->vehicleType());

    if (uRoute != vRoute)
    {
        auto const uProposal = Route::Proposal(uRoute->before(U->idx() - 1),
                                               uRoute->after(U->idx() + 1));

        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = fixedCost;
            costEvaluator.deltaCost(
                deltaCost,
                uProposal,
                Route::Proposal(vRoute->before(V->idx()),
                                uRoute->at(U->idx()),
                                ReloadDepotSegment(data, depot),
                                vRoute->after(V->idx() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::U_DEPOT, depot};
        }
    }
    else  // within same route
    {
        auto const *route = vRoute;
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = fixedCost;
            if (U->idx() < V->idx())
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(U->idx() - 1),
                                    route->between(U->idx() + 1, V->idx()),
                                    route->at(U->idx()),
                                    ReloadDepotSegment(data, depot),
                                    route->after(V->idx() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    Route::Proposal(route->before(V->idx()),
                                    route->at(U->idx()),
                                    ReloadDepotSegment(data, depot),
                                    route->between(V->idx() + 1, U->idx() - 1),
                                    route->after(U->idx() + 1)));

            if (deltaCost < move_.cost)
                move_ = {deltaCost, MoveType::U_DEPOT, depot};
        }
    }
}

pyvrp::Cost TripRelocate::evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (U == n(V) || vRoute->empty())  // if V's empty, Exchange<1, 0> suffices
        return 0;

    if (vRoute->numTrips() == vRoute->maxTrips())
        return 0;

    // Cannot evaluate this move because it requires a load segment to contain
    // a reload depot in the middle, which makes concatenation far more complex.
    if (uRoute == vRoute && U->trip() != V->trip())
        return 0;

    move_ = {};

    Cost fixedCost = 0;
    if (uRoute != vRoute && uRoute->numClients() == 1)  // empty after move
        fixedCost -= uRoute->fixedVehicleCost();

    if (!V->isDepot())
        evalDepotBefore(fixedCost, U, V, costEvaluator);

    if (!n(V)->isDepot())
        evalDepotAfter(fixedCost, U, V, costEvaluator);

    return move_.cost;
}

void TripRelocate::apply(Route::Node *U, Route::Node *V) const
{
    auto *uRoute = U->route();
    uRoute->remove(U->idx());

    auto *vRoute = V->route();
    Route::Node depot = {move_.depot};

    if (move_.type == MoveType::DEPOT_U)
    {
        vRoute->insert(V->idx() + 1, U);
        vRoute->insert(V->idx() + 1, &depot);
    }

    // We need to be careful to insert the depot last, because doing so could
    // invalidate V (it might trigger an update to the route's internal data
    // layout, which could invalidate V if V is a depot).
    if (move_.type == MoveType::U_DEPOT)
    {
        vRoute->insert(V->idx() + 1, U);
        vRoute->insert(V->idx() + 2, &depot);
    }
}
