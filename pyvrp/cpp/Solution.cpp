#include "Solution.h"
#include "ProblemData.h"
#include "TimeWindowSegment.h"

#include <fstream>
#include <numeric>
#include <sstream>

using Client = int;
using Visits = std::vector<Client>;
using Routes = std::vector<Solution::Route>;

void Solution::evaluate(ProblemData const &data)
{
    Cost allPrizes = 0;
    for (size_t client = 1; client <= data.numClients(); ++client)
        allPrizes += data.client(client).prize;

    for (auto const &route : routes_)
    {
        // Whole solution statistics.
        numClients_ += route.size();
        prizes_ += route.prizes();
        distance_ += route.distance();
        timeWarp_ += route.timeWarp();
        excessLoad_ += route.excessLoad();
    }

    uncollectedPrizes_ = allPrizes - prizes_;
}

size_t Solution::numRoutes() const { return routes_.size(); }

size_t Solution::numClients() const { return numClients_; }

Routes const &Solution::getRoutes() const { return routes_; }

std::vector<std::pair<Client, Client>> const &Solution::getNeighbours() const
{
    return neighbours;
}

bool Solution::isFeasible() const { return !hasExcessLoad() && !hasTimeWarp(); }

bool Solution::hasExcessLoad() const { return excessLoad_ > 0; }

bool Solution::hasTimeWarp() const { return timeWarp_ > 0; }

Distance Solution::distance() const { return distance_; }

Load Solution::excessLoad() const { return excessLoad_; }

Cost Solution::prizes() const { return prizes_; }

Cost Solution::uncollectedPrizes() const { return uncollectedPrizes_; }

Duration Solution::timeWarp() const { return timeWarp_; }

void Solution::makeNeighbours()
{
    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

bool Solution::operator==(Solution const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the neighbours are all equal.
    // clang-format off
    return distance_ == other.distance_
        && excessLoad_ == other.excessLoad_
        && timeWarp_ == other.timeWarp_
        && routes_.size() == other.routes_.size()
        && neighbours == other.neighbours;
    // clang-format on
}

Solution::Solution(ProblemData const &data, XorShift128 &rng)
    : neighbours(data.numClients() + 1, {0, 0})
{
    // Shuffle clients (to create random routes)
    auto clients = std::vector<int>(data.numClients());
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect.
    auto const numVehicles = data.numVehicles();
    auto const numClients = data.numClients();
    auto const perVehicle = std::max<size_t>(numClients / numVehicles, 1);
    auto const perRoute = perVehicle + (numClients % numVehicles != 0);
    auto const numRoutes = (numClients + perRoute - 1) / perRoute;

    std::vector<std::vector<Client>> routes(numRoutes);
    for (size_t idx = 0; idx != numClients; ++idx)
        routes[idx / perRoute].push_back(clients[idx]);

    routes_.reserve(numRoutes);
    for (size_t idx = 0; idx != numRoutes; ++idx)
        routes_.emplace_back(data, routes[idx]);

    makeNeighbours();
    evaluate(data);
}

Solution::Solution(ProblemData const &data,
                   std::vector<std::vector<Client>> const &routes)
    : neighbours(data.numClients() + 1, {0, 0})
{
    if (routes.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    std::vector<size_t> visits(data.numClients() + 1, 0);
    for (auto const &route : routes)
        for (auto const client : route)
            visits[client]++;

    for (size_t client = 1; client <= data.numClients(); ++client)
    {
        if (data.client(client).required && visits[client] == 0)
        {
            std::ostringstream msg;
            msg << "Client " << client << " is required but not present.";
            throw std::runtime_error(msg.str());
        }

        if (visits[client] > 1)
        {
            std::ostringstream msg;
            msg << "Client " << client << " is visited more than once.";
            throw std::runtime_error(msg.str());
        }
    }

    // Only store non-empty routes
    routes_.reserve(routes.size());
    for (size_t idx = 0; idx != routes.size(); ++idx)
        if (!routes[idx].empty())
            routes_.emplace_back(data, routes[idx]);

    makeNeighbours();
    evaluate(data);
}

Solution::Route::Route(ProblemData const &data, Visits const visits)
    : visits_(std::move(visits)), centroid_({0, 0})
{
    if (visits_.empty())
        return;

    auto const &depot = data.depot();
    auto const &durMat = data.durationMatrix();
    auto const depotTws = TimeWindowSegment(0, depot);
    tws_ = depotTws;
    int prevClient = 0;
    for (size_t idx = 0; idx != size(); ++idx)
    {
        auto const client = visits_[idx];
        auto const &clientData = data.client(client);

        distance_ += data.dist(prevClient, client);
        travel_ += data.duration(prevClient, client);
        demand_ += clientData.demand;
        service_ += clientData.serviceDuration;
        prizes_ += clientData.prize;

        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();

        auto const clientTws = TimeWindowSegment(client, clientData);
        tws_ = TimeWindowSegment::merge(durMat, tws_, clientTws);

        prevClient = client;
    }

    Client const last = visits_.back();  // last client has depot as successor
    distance_ += data.dist(last, 0);
    travel_ += data.duration(last, 0);
    tws_ = TimeWindowSegment::merge(durMat, tws_, depotTws);
    excessLoad_ = data.vehicleCapacity() < demand_
                      ? demand_ - data.vehicleCapacity()
                      : 0;
}

bool Solution::Route::empty() const { return visits_.empty(); }

size_t Solution::Route::size() const { return visits_.size(); }

Client Solution::Route::operator[](size_t idx) const { return visits_[idx]; }

Visits::const_iterator Solution::Route::begin() const
{
    return visits_.cbegin();
}

Visits::const_iterator Solution::Route::end() const { return visits_.cend(); }

Visits::const_iterator Solution::Route::cbegin() const
{
    return visits_.cbegin();
}

Visits::const_iterator Solution::Route::cend() const { return visits_.cend(); }

Visits const &Solution::Route::visits() const { return visits_; }

Distance Solution::Route::distance() const { return distance_; }

Load Solution::Route::demand() const { return demand_; }

Load Solution::Route::excessLoad() const { return excessLoad_; }

Duration Solution::Route::travelDuration() const { return travel_; }

Duration Solution::Route::serviceDuration() const { return service_; }

Duration Solution::Route::timeWarp() const { return tws_.timeWarp(); }

Duration Solution::Route::waitDuration() const
{
    return tws_.duration() - travel_ - service_;
}

Duration Solution::Route::duration() const { return tws_.duration(); }

Duration Solution::Route::earliestStart() const { return tws_.twEarly(); }

Duration Solution::Route::latestStart() const { return tws_.twLate(); }

Duration Solution::Route::slack() const
{
    return tws_.twLate() - tws_.twEarly();
}

Cost Solution::Route::prizes() const { return prizes_; }

std::pair<double, double> const &Solution::Route::centroid() const
{
    return centroid_;
}

bool Solution::Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Solution::Route::hasExcessLoad() const { return excessLoad_ > 0; }

bool Solution::Route::hasTimeWarp() const { return tws_.timeWarp() > 0; }

std::ostream &operator<<(std::ostream &out, Solution const &sol)
{
    auto const &routes = sol.getRoutes();

    for (size_t idx = 0; idx != routes.size(); ++idx)
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';

    return out;
}

std::ostream &operator<<(std::ostream &out, Solution::Route const &route)
{
    for (Client const client : route)
        out << client << ' ';
    return out;
}
