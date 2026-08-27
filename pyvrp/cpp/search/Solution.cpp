#include "Solution.h"

#include "ClientSegment.h"
#include "DeliverySegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "PickupSegment.h"
#include "Route.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <ostream>
#include <vector>

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::Solution;

namespace
{
/**
 * Segment that tracks a route segment between ``[start, end]``, but does so
 * incrementally: it starts from a single node and can be grown to cover more
 * of the route via its prefix operators.
 */
class IncrementalSegmentBetween : public Route::SegmentBetween
{
    pyvrp::ProblemData const &data_;
    pyvrp::DurationSegment duration_;
    std::vector<pyvrp::LoadSegment> loads_;

public:
    IncrementalSegmentBetween(pyvrp::ProblemData const &data,
                              Route::Node const *node);

    IncrementalSegmentBetween &operator++();

    inline pyvrp::DurationSegment const &duration(size_t profile) const;
    inline pyvrp::LoadSegment const &load(size_t dimension) const;
};

IncrementalSegmentBetween::IncrementalSegmentBetween(
    pyvrp::ProblemData const &data, Route::Node const *node)
    : SegmentBetween(*node->route(), node->pos(), node->pos()), data_(data)
{
    assert(node->route());
    duration_ = SegmentBetween::duration(route_.profile());

    loads_.reserve(data_.numLoadDimensions());
    for (size_t dim = 0; dim != data_.numLoadDimensions(); ++dim)
        loads_.emplace_back(SegmentBetween::load(dim));
}

IncrementalSegmentBetween &IncrementalSegmentBetween::operator++()
{
    assert(end != route_.size() - 1);
    auto const from = back().location();  // current last location
    end++;

    // The segment must consist of a single trip only, possibly including the
    // depot that begins the next trip (and ends this one). So the difference
    // in trips is at most one.
    assert(route_[end]->trip() - route_[start]->trip()
           <= route_[end]->isDepot());

    auto const at = route_.at(end);

    for (size_t dim = 0; dim != loads_.size(); ++dim)
        loads_[dim] = pyvrp::LoadSegment::merge(loads_[dim], at.load(dim));

    auto const &mat = data_.durationMatrix(route_.profile());
    auto const to = at.front().location();

    duration_ = pyvrp::DurationSegment::merge(
        mat(from, to), duration_, at.duration(route_.profile()));

    return *this;
}

pyvrp::DurationSegment const &
IncrementalSegmentBetween::duration([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return duration_;
}

pyvrp::LoadSegment const &
IncrementalSegmentBetween::load(size_t dimension) const
{
    return loads_[dimension];
}

Cost insertCost(pyvrp::search::Route::Node *U,
                pyvrp::search::Route::Node *V,
                pyvrp::ProblemData const &data,
                pyvrp::CostEvaluator const &costEvaluator)
{
    assert(V->route() && U->isClient());

    auto *route = V->route();
    auto const &client = data.client(U->idx());

    Cost deltaCost = -client.prize;
    costEvaluator.deltaCost<true>(
        deltaCost,
        pyvrp::search::Route::Proposal(
            route->before(V->pos()),
            pyvrp::search::ClientSegment(data, U->idx()),
            route->after(V->pos() + 1)));

    return deltaCost;
}

// Comparison operator to determine if pyvrp::Route and search::Route are
// equivalent - if so, the pyvrp::Route does not need to be loaded.
bool operator==(pyvrp::Route const &pyvrp, pyvrp::search::Route const &search)
{
    // clang-format off
    bool const simpleChecks = pyvrp.distance() == search.distance()
                              && pyvrp.duration() == search.duration()
                              && pyvrp.timeWarp() == search.timeWarp()
                              && pyvrp.vehicleType() == search.vehicleType()
                              && pyvrp.size() == search.size();
    // clang-format on

    if (!simpleChecks)
        return false;

    size_t idx = 0;
    for (auto const &activity : pyvrp)
        if (search[idx++]->activity() != activity)
            return false;

    return true;
}
}  // namespace

Solution::Solution(ProblemData const &data) : data_(data)
{
    clients.reserve(data.numClients());
    for (size_t client = 0; client != data.numClients(); ++client)
        clients.emplace_back(Activity::ActivityType::CLIENT, client);

    shipments.reserve(data.numShipments());
    for (size_t shipment = 0; shipment != data.numShipments(); ++shipment)
        shipments.push_back({{Activity::ActivityType::PICKUP, shipment},
                             {Activity::ActivityType::DELIVERY, shipment}});

    routes.reserve(data.numVehicles());
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data.vehicleType(vehType).numAvailable;
        for (size_t vehicle = 0; vehicle != numAvailable; ++vehicle)
            routes.emplace_back(data, vehType);
    }
}

void Solution::load(pyvrp::Solution const &solution)
{
    loadedSolution_ = &solution;

    // Determine offsets for vehicle types.
    std::vector<size_t> vehicleOffset(data_.numVehicleTypes(), 0);
    for (size_t vehType = 1; vehType < data_.numVehicleTypes(); vehType++)
    {
        auto const prevAvail = data_.vehicleType(vehType - 1).numAvailable;
        vehicleOffset[vehType] = vehicleOffset[vehType - 1] + prevAvail;
    }

    for (auto const &solRoute : solution.routes())
    {
        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const idx = vehicleOffset[solRoute.vehicleType()]++;
        auto &route = routes[idx];

        if (route == solRoute)  // then the current route is still OK and we
            continue;           // can skip inserting and updating

        // Else we need to clear the route and insert the updated route from
        // the solution.
        route.clear();

        route.reserve(solRoute.size());
        for (size_t idx = 1; idx != solRoute.size() - 1; ++idx)
        {
            auto const &activity = solRoute[idx];
            if (auto *ptr = this->operator[](activity))  // client or shipment
                route.push_back(ptr);                    // visit
            else
            {                                 // an activity of which the route
                Route::Node node = activity;  // needs to take ownership
                route.push_back(&node);
            }
        }

        route.update();
    }

    // Finally, we clear any routes that we have not re-used or inserted from
    // the solution.
    size_t firstOfType = 0;
    for (size_t vehType = 0; vehType != data_.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data_.vehicleType(vehType).numAvailable;
        auto const firstOfNextType = firstOfType + numAvailable;
        for (size_t idx = vehicleOffset[vehType]; idx != firstOfNextType; ++idx)
            routes[idx].clear();

        firstOfType = firstOfNextType;
    }
}

pyvrp::Solution Solution::unload() const
{
    // Determine offsets for vehicle types.
    std::vector<size_t> vehicleOffset(data_.numVehicleTypes(), 0);
    for (size_t vehType = 1; vehType != data_.numVehicleTypes(); ++vehType)
    {
        auto const prevAvail = data_.vehicleType(vehType - 1).numAvailable;
        vehicleOffset[vehType] = vehicleOffset[vehType - 1] + prevAvail;
    }

    // Map each route to the route it was loaded from (if any), following the
    // same route layout as load().
    std::vector<pyvrp::Route const *> loadedRoutes(routes.size());
    if (loadedSolution_)
        for (auto const &solRoute : loadedSolution_->routes())
            loadedRoutes[vehicleOffset[solRoute.vehicleType()]++] = &solRoute;

    std::vector<pyvrp::Route> solRoutes;
    solRoutes.reserve(data_.numVehicles());

    for (size_t idx = 0; idx != routes.size(); ++idx)
    {
        auto const &route = routes[idx];
        if (route.empty())
            continue;

        // If the route is unchanged since loading, we copy the loaded route
        // rather than rebuild it.
        auto const *solRoute = loadedRoutes[idx];
        if (solRoute && *solRoute == route)
        {
            solRoutes.push_back(*solRoute);
            continue;
        }

        std::vector<Activity> activities;
        activities.reserve(route.size());

        for (size_t idx = 1; idx != route.size() - 1; ++idx)
            activities.emplace_back(route[idx]->activity());

        solRoutes.emplace_back(
            data_, std::move(activities), route.vehicleType());
    }

    return {data_, std::move(solRoutes)};
}

bool Solution::insert(Route::Node *U,
                      SearchSpace const &searchSpace,
                      CostEvaluator const &costEvaluator,
                      bool required)
{
    assert(U->isClient() && !U->route());

    Route::Node *UAfter = routes[0][0];  // fallback option
    auto bestCost = insertCost(U, UAfter, data_, costEvaluator);

    // First attempt a neighbourhood search to place U into routes that are
    // already in use.
    for (auto const &vActivity : searchSpace.neighboursOf(U->activity()))
    {
        Route::Node *V = this->operator[](vActivity);
        assert(V);

        if (!V->route())
            continue;

        auto const cost = insertCost(U, V, data_, costEvaluator);
        if (cost < bestCost)
        {
            bestCost = cost;
            UAfter = V;
        }
    }

    // Next consider empty routes, of each vehicle type. We insert into the
    // first improving route.
    for (auto const &[vehType, offset] : searchSpace.vehTypeOrder())
    {
        auto const begin = routes.begin() + offset;
        auto const end = begin + data_.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty == end)
            continue;

        auto const cost = insertCost(U, (*empty)[0], data_, costEvaluator);
        if (cost < bestCost)
        {
            bestCost = cost;
            UAfter = (*empty)[0];
            break;
        }
    }

    if (required || bestCost < 0)
    {
        auto *route = UAfter->route();
        route->insert(UAfter->pos() + 1, U);
        return true;
    }

    return false;
}

bool Solution::insert(Route::Node *pickup,
                      Route::Node *delivery,
                      SearchSpace const &searchSpace,
                      CostEvaluator const &costEvaluator,
                      bool required)
{
    assert(pickup->isPickup() && delivery->isDelivery());
    assert(pickup->idx() == delivery->idx());
    assert(!pickup->route() && !delivery->route());

    auto const &shipment = data_.shipment(pickup->idx());

    Route::Node *pickupAfter = routes[0][0];  // fallback option
    size_t deliveryPos = 1;
    Cost bestCost = std::numeric_limits<Cost>::max();

    // First we search the shipment's neighbourhood to insert the pickup and
    // delivery in a route that's already in use.
    for (auto const &vActivity : searchSpace.neighboursOf(pickup->activity()))
    {
        Route::Node *neighbour = this->operator[](vActivity);
        assert(neighbour);

        auto const *route = neighbour->route();
        if (!route)
            continue;

        for (auto *V : {p(neighbour), neighbour})  // before or after neighbour
        {
            Cost deltaCost = -shipment.prize;
            costEvaluator.deltaCost<true>(
                deltaCost,  // delivery directly after pickup
                Route::Proposal(route->before(V->pos()),
                                PickupSegment(data_, pickup->idx()),
                                DeliverySegment(data_, delivery->idx()),
                                route->after(V->pos() + 1)));

            if (deltaCost < bestCost)
            {
                pickupAfter = V;
                deliveryPos = V->pos() + 1;
                bestCost = deltaCost;
            }

            IncrementalSegmentBetween between = {data_, n(V)};
            for (auto const *node = n(V); !node->isDepot();
                 node = n(node), ++between)
            {
                Cost deltaCost = -shipment.prize;
                costEvaluator.deltaCost<true>(
                    deltaCost,
                    Route::Proposal(route->before(V->pos()),
                                    PickupSegment(data_, pickup->idx()),
                                    between,
                                    DeliverySegment(data_, delivery->idx()),
                                    route->after(node->pos() + 1)));

                if (deltaCost < bestCost)
                {
                    pickupAfter = V;
                    deliveryPos = node->pos() + 1;
                    bestCost = deltaCost;
                }
            }
        }
    }

    // Finally, we consider inserting into an empty route. We insert into the
    // first improving one.
    for (auto const &[vehType, offset] : searchSpace.vehTypeOrder())
    {
        auto const begin = routes.begin() + offset;
        auto const end = begin + data_.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty == end)
            continue;

        Cost deltaCost = -shipment.prize;
        costEvaluator.deltaCost<true>(
            deltaCost,
            Route::Proposal(empty->before(0),
                            PickupSegment(data_, pickup->idx()),
                            DeliverySegment(data_, delivery->idx()),
                            empty->after(1)));

        if (deltaCost < bestCost)
        {
            pickupAfter = (*empty)[0];
            deliveryPos = 1;
            bestCost = deltaCost;
            break;
        }
    }

    if (required || bestCost < 0)
    {
        auto *route = pickupAfter->route();
        route->insert(deliveryPos, delivery);
        route->insert(pickupAfter->pos() + 1, pickup);
        return true;
    }

    return false;
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Solution const &sol)
{
    for (size_t idx = 0; idx != sol.routes.size(); ++idx)
        out << "Route #" << idx + 1 << ": " << sol.routes[idx] << '\n';
    return out;
}

template <>
pyvrp::Cost pyvrp::CostEvaluator::penalisedCost(
    pyvrp::search::Solution const &solution) const
{
    auto const &data = solution.data_;

    Cost cost = 0;  // cost is route cost + uncollected prizes
    for (size_t idx = 0; idx != data.numClients(); ++idx)
        if (!solution.clients[idx].route())
            cost += data.client(idx).prize;

    for (size_t idx = 0; idx != data.numShipments(); ++idx)
        if (!solution.shipments[idx].first.route())
            cost += data.shipment(idx).prize;

    for (auto const &route : solution.routes)
        cost += penalisedCost(route);

    return cost;
}
