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
    bool isReloadDepot() const { return true; }

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

pyvrp::Cost
TripRelocate::evaluate(Route::Node *U,
                       Route::Node *V,
                       [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (vRoute->numTrips() == vRoute->maxTrips() || !vRoute->hasExcessLoad())
        // Then we cannot insert another depot in V, or V is already load
        // feasible, so we have nothing to do. It suffices to check the load
        // feasibility of V rather than also U, because Exchange<1, 0> already
        // covers the case of moving U without inserting an additional depot.
        return 0;

    Cost deltaCost = 0;

    if (uRoute->numClients() == 1)
        deltaCost -= uRoute->fixedVehicleCost();

    if (vRoute->empty())
        deltaCost += vRoute->fixedVehicleCost();

    auto const uProposal = Route::Proposal(uRoute->before(U->idx() - 1),
                                           uRoute->after(U->idx() + 1));

    auto const &vehType = data.vehicleType(vRoute->vehicleType());
    for (auto const depot : vehType.reloadDepots)
    {
        auto const vBefore = Route::Proposal(vRoute->before(V->idx()),
                                             ReloadDepotSegment(data, depot),
                                             uRoute->at(U->idx()),
                                             vRoute->after(V->idx() + 1));

        auto tmpCost = deltaCost;
        costEvaluator.deltaCost(tmpCost, uProposal, vBefore);
        if (tmpCost < 0)
        {
            move = {MoveType::DEPOT_U, depot};
            return tmpCost;
        }

        auto const vAfter = Route::Proposal(vRoute->before(V->idx()),
                                            uRoute->at(U->idx()),
                                            ReloadDepotSegment(data, depot),
                                            vRoute->after(V->idx() + 1));

        tmpCost = deltaCost;
        costEvaluator.deltaCost(tmpCost, uProposal, vAfter);
        if (tmpCost < 0)
        {
            move = {MoveType::U_DEPOT, depot};
            return tmpCost;
        }
    }

    return 0;
}

void TripRelocate::apply([[maybe_unused]] Route::Node *U,
                         [[maybe_unused]] Route::Node *V) const
{
    if (move.type == MoveType::DEPOT_U)
    {
        // TODO
    }

    if (move.type == MoveType::U_DEPOT)
    {
        // TODO
    }
}
