#include "Solution.h"
#include "DurationSegment.h"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <unordered_map>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;
using pyvrp::Solution;

using Client = size_t;
using Routes = std::vector<Route>;
using Neighbours = std::vector<std::optional<std::pair<Client, Client>>>;

void Solution::evaluate(ProblemData const &data)
{
    Cost allPrizes = 0;
    for (auto const &client : data.clients())
        allPrizes += client.prize;

    excessLoad_ = std::vector<Load>(data.numLoadDimensions(), 0);
    for (auto const &route : routes_)
    {
        // Whole solution statistics.
        numClients_ += route.size();
        prizes_ += route.prizes();
        distance_ += route.distance();
        distanceCost_ += route.distanceCost();
        duration_ += route.duration();
        durationCost_ += route.durationCost();
        excessDistance_ += route.excessDistance();
        timeWarp_ += route.timeWarp();
        fixedVehicleCost_ += data.vehicleType(route.vehicleType()).fixedCost;

        auto const &excessLoad = route.excessLoad();
        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
            excessLoad_[dim] += excessLoad[dim];
    }

    uncollectedPrizes_ = allPrizes - prizes_;
}

bool Solution::empty() const { return numClients() == 0 && numRoutes() == 0; }

size_t Solution::numRoutes() const { return routes_.size(); }

size_t Solution::numClients() const { return numClients_; }

size_t Solution::numMissingClients() const { return numMissingClients_; }

Routes const &Solution::routes() const { return routes_; }

Neighbours const &Solution::neighbours() const { return neighbours_; }

bool Solution::isFeasible() const
{
    // clang-format off
    return !hasExcessLoad()
        && !hasTimeWarp()
        && !hasExcessDistance()
        && isComplete()
        && isGroupFeasible();
    // clang-format on
}

bool Solution::isGroupFeasible() const { return isGroupFeas_; }

bool Solution::isComplete() const { return numMissingClients_ == 0; }

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

Cost Solution::durationCost() const { return durationCost_; }

std::vector<Load> const &Solution::excessLoad() const { return excessLoad_; }

Distance Solution::excessDistance() const { return excessDistance_; }

Cost Solution::fixedVehicleCost() const { return fixedVehicleCost_; }

Cost Solution::prizes() const { return prizes_; }

Cost Solution::uncollectedPrizes() const { return uncollectedPrizes_; }

Duration Solution::timeWarp() const { return timeWarp_; }

void Solution::makeNeighbours(ProblemData const &data)
{
    for (auto const &route : routes_)
    {
        auto const &vehicleType = data.vehicleType(route.vehicleType());
        auto const startDepot = vehicleType.startDepot;
        auto const endDepot = vehicleType.endDepot;

        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours_[route[idx]] = {
                idx == 0 ? startDepot : route[idx - 1],                // pred
                idx == route.size() - 1 ? endDepot : route[idx + 1]};  // succ
    }
}

bool Solution::operator==(Solution const &other) const
{
    // clang-format off
    bool const attributeChecks = distance_ == other.distance_
                              && duration_ == other.duration_
                              && distanceCost_ == other.distanceCost_
                              && durationCost_ == other.durationCost_
                              && timeWarp_ == other.timeWarp_
                              && isGroupFeas_ == other.isGroupFeas_
                              && routes_.size() == other.routes_.size()
                              && neighbours_ == other.neighbours_;
    // clang-format on

    if (!attributeChecks)
        return false;

    // The visits are the same for both solutions, but the vehicle assignments
    // need not be. We check this via a mapping from the first client in each
    // route to the vehicle type of that route. We need to base this on the
    // visits since the route order can differ between solutions.
    std::unordered_map<Client, VehicleType> client2vehType;
    for (auto const &route : routes_)
        client2vehType[route[0]] = route.vehicleType();

    for (auto const &route : other.routes_)
        if (client2vehType[route[0]] != route.vehicleType())
            return false;

    return true;
}

Solution::Solution(ProblemData const &data, RandomNumberGenerator &rng)
    : neighbours_(data.numLocations(), std::nullopt)
{
    // Add all required and randomly selected optional clients.
    std::vector<size_t> clients;
    clients.reserve(data.numClients());
    for (size_t idx = data.numDepots(); idx != data.numLocations(); ++idx)
    {
        pyvrp::ProblemData::Client const &clientData = data.location(idx);
        if (clientData.required || rng.rand() < 0.5)
            clients.push_back(idx);
    }

    // Shuffle clients to create random routes.
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect and
    // there are not enough vehicles for single-client routes.
    auto const numVehicles = data.numVehicles();
    auto const numClients = clients.size();
    auto const perVehicle = std::max<size_t>(numClients / numVehicles, 1);
    auto const adjustment
        = numClients > numVehicles && numClients % numVehicles != 0;
    auto const perRoute = perVehicle + adjustment;
    auto const numRoutes = (numClients + perRoute - 1) / perRoute;

    std::vector<std::vector<Client>> routes(numRoutes);
    for (size_t idx = 0; idx != numClients; ++idx)
        routes[idx / perRoute].push_back(clients[idx]);

    std::vector<size_t> vehTypes;
    vehTypes.reserve(data.numVehicles());
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data.vehicleType(vehType).numAvailable;
        std::fill_n(std::back_inserter(vehTypes), numAvailable, vehType);
    }

    if (data.numVehicleTypes() > 1)
        // Shuffle vehicle types when there is more than one. This ensures some
        // additional diversity in the initial solutions, which sometimes (e.g.
        // with heterogeneous fleet VRP) matters for consistent convergence.
        std::shuffle(vehTypes.begin(), vehTypes.end(), rng);

    routes_.reserve(numRoutes);
    for (size_t idx = 0; idx != routes.size(); idx++)
        routes_.emplace_back(data, routes[idx], vehTypes[idx]);

    *this = Solution(data, routes_);
}

Solution::Solution(ProblemData const &data,
                   std::vector<std::vector<Client>> const &routes)
{
    Routes transformedRoutes;
    transformedRoutes.reserve(routes.size());
    for (auto const &visits : routes)
        transformedRoutes.emplace_back(data, visits, 0);

    *this = Solution(data, transformedRoutes);
}

Solution::Solution(ProblemData const &data, std::vector<Route> const &routes)
    : routes_(routes), neighbours_(data.numLocations(), std::nullopt)
{
    if (routes.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    std::vector<size_t> visits(data.numLocations(), 0);
    std::vector<size_t> usedVehicles(data.numVehicleTypes(), 0);
    for (auto const &route : routes)
    {
        if (route.empty())
            throw std::runtime_error("Solution should not have empty routes.");

        usedVehicles[route.vehicleType()]++;
        for (auto const client : route)
            visits[client]++;
    }

    for (size_t client = data.numDepots(); client != data.numLocations();
         ++client)
    {
        ProblemData::Client const &clientData = data.location(client);
        if (clientData.required && visits[client] == 0)
            numMissingClients_ += 1;

        if (visits[client] > 1)
        {
            std::ostringstream msg;
            msg << "Client " << client << " is visited more than once.";
            throw std::runtime_error(msg.str());
        }
    }

    for (auto const &group : data.groups())
    {
        // The solution is feasible w.r.t. this client group if exactly one of
        // the clients in the group is in the solution. When the group is not
        // required, we relax this to at most one client.
        assert(group.mutuallyExclusive);
        auto const inSol = [&](auto client) { return visits[client] == 1; };
        auto const numInSol = std::count_if(group.begin(), group.end(), inSol);
        isGroupFeas_ &= group.required ? numInSol == 1 : numInSol <= 1;
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

    makeNeighbours(data);
    evaluate(data);
}

Solution::Solution(size_t numClients,
                   size_t numMissingClients,
                   Distance distance,
                   Cost distanceCost,
                   Duration duration,
                   Cost durationCost,
                   Distance excessDistance,
                   std::vector<Load> excessLoad,
                   Cost fixedVehicleCost,
                   Cost prizes,
                   Cost uncollectedPrizes,
                   Duration timeWarp,
                   bool isGroupFeasible,
                   Routes routes,
                   Neighbours neighbours)
    : numClients_(numClients),
      numMissingClients_(numMissingClients),
      distance_(distance),
      distanceCost_(distanceCost),
      duration_(duration),
      durationCost_(durationCost),
      excessDistance_(excessDistance),
      excessLoad_(std::move(excessLoad)),
      fixedVehicleCost_(fixedVehicleCost),
      prizes_(prizes),
      uncollectedPrizes_(uncollectedPrizes),
      timeWarp_(timeWarp),
      isGroupFeas_(isGroupFeasible),
      routes_(std::move(routes)),
      neighbours_(std::move(neighbours))
{
}

std::ostream &operator<<(std::ostream &out, Solution const &sol)
{
    auto const &routes = sol.routes();

    for (size_t idx = 0; idx != routes.size(); ++idx)
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';

    return out;
}
