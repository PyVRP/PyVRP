#include "Solution.h"
#include "DurationSegment.h"
#include "DynamicBitset.h"

#include <algorithm>
#include <fstream>
#include <numeric>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;
using pyvrp::Solution;

using Routes = std::vector<Route>;
using Unplanned = std::vector<pyvrp::Activity>;

void Solution::evaluate(ProblemData const &data)
{
    Cost allPrizes = 0;
    for (auto const &client : data.clients())
        allPrizes += client.prize;

    excessLoad_ = std::vector<Load>(data.numLoadDimensions(), 0);
    for (auto const &route : routes_)
    {
        // Whole solution statistics.
        numClients_ += route.numClients();
        numShipments_ += route.numShipments();
        prizes_ += route.prizes();
        distance_ += route.distance();
        distanceCost_ += route.distanceCost();
        duration_ += route.duration();
        overtime_ += route.overtime();
        durationCost_ += route.durationCost();
        excessDistance_ += route.excessDistance();
        timeWarp_ += route.timeWarp();
        fixedVehicleCost_ += route.fixedVehicleCost();

        auto const &excessLoad = route.excessLoad();
        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
            excessLoad_[dim] += excessLoad[dim];
    }

    uncollectedPrizes_ = allPrizes - prizes_;
}

bool Solution::empty() const
{
    return numClients() == 0 && numShipments() == 0 && numRoutes() == 0;
}

size_t Solution::numRoutes() const { return routes_.size(); }

size_t Solution::numTrips() const
{
    return std::accumulate(routes_.begin(),
                           routes_.end(),
                           0,
                           [](size_t count, auto const &route)
                           { return count + route.numTrips(); });
}

size_t Solution::numClients() const { return numClients_; }

size_t Solution::numShipments() const { return numShipments_; }

size_t Solution::numMissingClients() const { return numMissingClients_; }

size_t Solution::numMissingGroups() const { return numMissingGroups_; }

size_t Solution::numMissingShipments() const { return numMissingShipments_; }

Routes const &Solution::routes() const { return routes_; }

Unplanned const &Solution::unplanned() const { return unplanned_; }

bool Solution::isFeasible() const
{
    // clang-format off
    return !hasExcessLoad()
        && !hasTimeWarp()
        && !hasExcessDistance()
        && isComplete();
    // clang-format on
}

bool Solution::isComplete() const
{
    // clang-format off
    return numMissingClients_ == 0
        && numMissingGroups_ == 0
        && numMissingShipments_ == 0;
    // clang-format on
}

bool Solution::hasExcessLoad() const
{
    return std::any_of(excessLoad_.begin(),
                       excessLoad_.end(),
                       [](auto const excess) { return excess > 0; });
}

bool Solution::hasExcessDistance() const { return excessDistance_ > 0; }

bool Solution::hasTimeWarp() const { return timeWarp_ > 0; }

Distance Solution::distance() const { return distance_; }

Cost Solution::distanceCost() const { return distanceCost_; }

Duration Solution::duration() const { return duration_; }

Duration Solution::overtime() const { return overtime_; }

Cost Solution::durationCost() const { return durationCost_; }

std::vector<Load> const &Solution::excessLoad() const { return excessLoad_; }

Distance Solution::excessDistance() const { return excessDistance_; }

Cost Solution::fixedVehicleCost() const { return fixedVehicleCost_; }

Cost Solution::prizes() const { return prizes_; }

Cost Solution::uncollectedPrizes() const { return uncollectedPrizes_; }

Duration Solution::timeWarp() const { return timeWarp_; }

bool Solution::operator==(Solution const &other) const
{
    // clang-format off
    bool const attributeChecks = distance_ == other.distance_
                              && duration_ == other.duration_
                              && distanceCost_ == other.distanceCost_
                              && durationCost_ == other.durationCost_
                              && timeWarp_ == other.timeWarp_
                              && numClients_ == other.numClients_
                              && numShipments_ == other.numShipments_;
    // clang-format on

    if (!attributeChecks)
        return false;

    // Tests if the routes are permutations of each other. Quadratic (in the
    // number of routes) in the worst case.
    return std::is_permutation(routes_.begin(),
                               routes_.end(),
                               other.routes_.begin(),
                               other.routes_.end());
}

Solution::Solution(ProblemData const &data, RandomNumberGenerator &rng)
{
    // Add all required and randomly selected optional client and pickup
    // activities. For required groups we insert a random client, for optional
    // groups we choose randomly whether to insert at all.
    std::vector<Activity> activities;
    activities.reserve(data.numClients() + data.numShipments());

    for (auto const &group : data.groups())  // first handle groups
        if (group.required || rng.rand() < 0.5)
        {
            auto const &members = group.clients();
            auto const idx = rng.randint(members.size());
            activities.emplace_back(Activity::ActivityType::CLIENT,
                                    members[idx]);
        }

    for (size_t idx = 0; idx != data.numClients(); ++idx)
    {
        auto const &client = data.client(idx);
        if (client.group)  // already handled groups above, skip here
            continue;

        if (client.required || rng.rand() < 0.5)
            activities.emplace_back(Activity::ActivityType::CLIENT, idx);
    }

    for (size_t idx = 0; idx != data.numShipments(); ++idx)
    {
        auto const &shipment = data.shipment(idx);

        if (shipment.required || rng.rand() < 0.5)
            activities.emplace_back(Activity::ActivityType::PICKUP, idx);
    }

    // Shuffle the activities to create random routes.
    rng.shuffle(activities.begin(), activities.end());

    // Distribute activities evenly over the routes: the total number of
    // activities per vehicle, with an adjustment in case the division is not
    // perfect and there are not enough vehicles for singleton routes.
    auto const numVehicles = data.numVehicles();
    auto const numActivities = activities.size();
    auto const perVehicle = std::max<size_t>(numActivities / numVehicles, 1);
    auto const adjustment
        = numActivities > numVehicles && numActivities % numVehicles != 0;
    auto const perRoute = perVehicle + adjustment;
    auto const numRoutes = (numActivities + perRoute - 1) / perRoute;

    std::vector<std::vector<Activity>> routes(numRoutes);
    for (size_t idx = 0; idx != numActivities; ++idx)
    {
        auto const &activity = activities[idx];
        auto &route = routes[idx / perRoute];

        if (activity.isClient())
            route.emplace_back(activity);

        if (activity.isPickup())  // then we insert the pickup somewhere in the
        {                         // route, and emplace the delivery at the end
            auto const pos = rng.randint(route.size() + 1);
            route.insert(route.begin() + pos, activity);
            route.emplace_back(Activity::ActivityType::DELIVERY,
                               activity.idx());
        }
    }

    std::vector<size_t> vehTypes;
    vehTypes.reserve(data.numVehicles());
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data.vehicleType(vehType).numAvailable;
        std::fill_n(std::back_inserter(vehTypes), numAvailable, vehType);
    }

    if (data.numVehicleTypes() > 1)
        // Shuffle vehicle types when there is more than one. This ensures
        // some additional diversity in the initial solutions, which
        // sometimes (e.g. with heterogeneous fleet VRP) matters for
        // consistent convergence.
        rng.shuffle(vehTypes.begin(), vehTypes.end());

    routes_.reserve(numRoutes);
    for (size_t idx = 0; idx != routes.size(); idx++)
        routes_.emplace_back(data, routes[idx], vehTypes[idx]);

    *this = Solution(data, routes_);
}

Solution::Solution(ProblemData const &data,
                   std::vector<std::vector<size_t>> const &routes)
{
    Routes transformedRoutes;
    transformedRoutes.reserve(routes.size());
    for (auto const &visits : routes)
        transformedRoutes.emplace_back(data, visits, 0);

    *this = Solution(data, transformedRoutes);
}

Solution::Solution(ProblemData const &data, std::vector<Route> routes)
    : routes_(std::move(routes))
{
    if (routes_.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    DynamicBitset isClientVisited(data.numClients());
    DynamicBitset isShipmentVisited(data.numShipments());
    std::vector<size_t> usedVehicles(data.numVehicleTypes(), 0);
    for (auto const &route : routes_)
    {
        if (route.empty())
            throw std::runtime_error("Solution should not have empty routes.");

        static_assert(std::ranges::input_range<Route>);

        usedVehicles[route.vehicleType()]++;
        for (auto const &activity : route)
        {
            switch (activity.type())
            {
            case Activity::ActivityType::CLIENT:
            {
                auto const client = activity.idx();
                if (isClientVisited[client])
                {
                    std::ostringstream msg;
                    msg << "Client " << client << " is visited more than once.";
                    throw std::runtime_error(msg.str());
                }

                isClientVisited[client] = true;
                break;
            }

            case Activity::ActivityType::PICKUP:
            {
                auto const shipment = activity.idx();
                if (isShipmentVisited[shipment])  // then we've seen this
                {                                 // pickup before
                    std::ostringstream msg;
                    msg << "Shipment " << shipment
                        << " is visited more than once.";
                    throw std::runtime_error(msg.str());
                }

                isShipmentVisited[shipment] = true;
                break;
            }

            default:
                break;
            }
        }
    }

    for (size_t client = 0; client != data.numClients(); ++client)
        if (!isClientVisited[client])
        {
            auto const &clientData = data.client(client);
            numMissingClients_ += clientData.required;

            unplanned_.emplace_back(Activity::ActivityType::CLIENT, client);
        }

    for (size_t shipment = 0; shipment != data.numShipments(); ++shipment)
    {
        if (!isShipmentVisited[shipment])
        {
            auto const &shipmentData = data.shipment(shipment);
            numMissingShipments_ += shipmentData.required;

            unplanned_.emplace_back(Activity::ActivityType::PICKUP, shipment);
            unplanned_.emplace_back(Activity::ActivityType::DELIVERY, shipment);
        }
    }

    for (size_t idx = 0; idx != data.numGroups(); ++idx)
    {
        auto const &group = data.group(idx);
        assert(group.mutuallyExclusive);

        auto const inSol = [&](auto client) { return isClientVisited[client]; };
        auto const count = std::count_if(group.begin(), group.end(), inSol);
        if (count > 1)
        {
            std::ostringstream msg;
            msg << "Group " << idx << " is visited more than once.";
            throw std::runtime_error(msg.str());
        }

        if (group.required && count == 0)  // required but missing group
            numMissingGroups_++;
    }

    for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
        if (usedVehicles[vehType] > data.vehicleType(vehType).numAvailable)
        {
            std::ostringstream msg;
            auto const numAvailable = data.vehicleType(vehType).numAvailable;
            msg << "Used more than " << numAvailable << " vehicles of type "
                << vehType << '.';
            throw std::runtime_error(msg.str());
        }

    evaluate(data);
}

Solution::Solution(size_t numClients,
                   size_t numShipments,
                   size_t numMissingClients,
                   size_t numMissingGroups,
                   size_t numMissingShipments,
                   Distance distance,
                   Cost distanceCost,
                   Duration duration,
                   Duration overtime,
                   Cost durationCost,
                   Distance excessDistance,
                   std::vector<Load> excessLoad,
                   Cost fixedVehicleCost,
                   Cost prizes,
                   Cost uncollectedPrizes,
                   Duration timeWarp,
                   Routes routes)
    : numClients_(numClients),
      numShipments_(numShipments),
      numMissingClients_(numMissingClients),
      numMissingGroups_(numMissingGroups),
      numMissingShipments_(numMissingShipments),
      distance_(distance),
      distanceCost_(distanceCost),
      duration_(duration),
      overtime_(overtime),
      durationCost_(durationCost),
      excessDistance_(excessDistance),
      excessLoad_(std::move(excessLoad)),
      fixedVehicleCost_(fixedVehicleCost),
      prizes_(prizes),
      uncollectedPrizes_(uncollectedPrizes),
      timeWarp_(timeWarp),
      routes_(std::move(routes))
{
}

template <>
Cost pyvrp::CostEvaluator::penalisedCost(Solution const &solution) const
{
    Cost cost = solution.uncollectedPrizes();
    for (auto const &route : solution.routes())
        cost += penalisedCost(route);

    return cost;
}

std::ostream &operator<<(std::ostream &out, Solution const &sol)
{
    auto const &routes = sol.routes();

    for (size_t idx = 0; idx != routes.size(); ++idx)
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';

    return out;
}
