#include "TripRelocate.h"

#include "Route.h"

#include <cassert>
#include <type_traits>

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
    constexpr std::true_type isReloadDepot() const { return std::true_type(); }

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
        return {depot, depot.serviceDuration};
    }

    pyvrp::LoadSegment load([[maybe_unused]] size_t dimension) const
    {
        return {};
    }
};
}  // namespace

pyvrp::Cost TripRelocate::evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (vRoute->numTrips() == vRoute->maxTrips() || !vRoute->hasExcessLoad())
        // Then we cannot insert another depot in V, or V is already load
        // feasible. We have nothing to do in either case.
        return 0;

    move = {};

    if (uRoute != vRoute)
    {
        Cost deltaCost = 0;

        if (uRoute->numClients() == 1)  // will become empty after move
            deltaCost -= uRoute->fixedVehicleCost();

        if (vRoute->empty())  // no longer empty after move
            deltaCost += vRoute->fixedVehicleCost();

        auto const uProposal = Route::Proposal(uRoute->before(U->idx() - 1),
                                               uRoute->after(U->idx() + 1));

        auto const &vehType = data.vehicleType(vRoute->vehicleType());
        for (auto const depot : vehType.reloadDepots)
        {
            Route::Proposal const vDepotBefore
                = {vRoute->before(V->idx()),
                   ReloadDepotSegment(data, depot),
                   uRoute->at(U->idx()),
                   vRoute->after(V->idx() + 1)};

            auto tmpCost = deltaCost;
            costEvaluator.deltaCost(tmpCost, uProposal, vDepotBefore);
            if (tmpCost < move.cost)
                move = {tmpCost, MoveType::DEPOT_U, depot};

            Route::Proposal const vDepotAfter
                = {vRoute->before(V->idx()),
                   uRoute->at(U->idx()),
                   ReloadDepotSegment(data, depot),
                   vRoute->after(V->idx() + 1)};

            tmpCost = deltaCost;
            costEvaluator.deltaCost(tmpCost, uProposal, vDepotAfter);
            if (tmpCost < move.cost)
                move = {tmpCost, MoveType::U_DEPOT, depot};
        }
    }
    else  // within same route
    {
        auto const *route = vRoute;
        auto const &vehType = data.vehicleType(route->vehicleType());
        for (auto const depot : vehType.reloadDepots)
        {
            Cost deltaCost = 0;

            if (U->idx() < V->idx())  // insert depot before U
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

            if (deltaCost < move.cost)
                move = {deltaCost, MoveType::DEPOT_U, depot};

            deltaCost = 0;
            if (U->idx() < V->idx())  // insert depot after U
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

            if (deltaCost < move.cost)
                move = {deltaCost, MoveType::U_DEPOT, depot};
        }
    }

    return move.cost;
}

void TripRelocate::apply(Route::Node *U, Route::Node *V) const
{
    auto *uRoute = U->route();
    uRoute->remove(U->idx());

    auto *vRoute = V->route();
    Route::Node depot = {move.depot};

    if (move.type == MoveType::DEPOT_U)
    {
        vRoute->insert(V->idx() + 1, U);
        vRoute->insert(V->idx() + 1, &depot);
    }

    if (move.type == MoveType::U_DEPOT)
    {
        vRoute->insert(V->idx() + 1, &depot);
        vRoute->insert(V->idx() + 1, U);
    }
}
