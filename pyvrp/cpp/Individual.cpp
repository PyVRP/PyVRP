#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>
#include <vector>

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

void Individual::evaluate(ProblemData const &data)
{
    uncollected_ = 0;
    for (size_t client = 0; client <= data.numClients(); ++client)
        uncollected_ += data.client(client).prize;

    numRoutes_ = 0;
    size_ = 0;
    distance_ = 0;
    excessLoad_ = 0;
    timeWarp_ = 0;

    for (auto const &route : routes_)
    {
        if (route.empty())  // First empty route. All subsequent routes are
            break;          // empty as well.

        numRoutes_++;
        size_ += route.size();

        int routeDist = data.dist(0, route[0]);
        int routeTimeWarp = 0;
        int routeLoad = data.client(route[0]).demand;
        int routePrize = data.client(route[0]).prize;

        int time = data.duration(0, route[0]);

        if (time < data.client(route[0]).twEarly)
            time = data.client(route[0]).twEarly;

        if (time > data.client(route[0]).twLate)
        {
            routeTimeWarp += time - data.client(route[0]).twLate;
            time = data.client(route[0]).twLate;
        }

        for (size_t idx = 1; idx < route.size(); idx++)
        {
            routeDist += data.dist(route[idx - 1], route[idx]);
            routeLoad += data.client(route[idx]).demand;
            routePrize += data.client(route[idx]).prize;

            time += data.client(route[idx - 1]).serviceDuration
                    + data.duration(route[idx - 1], route[idx]);

            // Add possible waiting time
            if (time < data.client(route[idx]).twEarly)
                time = data.client(route[idx]).twEarly;

            // Add possible time warp
            if (time > data.client(route[idx]).twLate)
            {
                routeTimeWarp += time - data.client(route[idx]).twLate;
                time = data.client(route[idx]).twLate;
            }
        }

        // For the last client, the successors is the depot. Also update the
        // rDist and time
        routeDist += data.dist(route.back(), 0);
        time += data.client(route.back()).serviceDuration
                + data.duration(route.back(), 0);

        // For the depot, we only need to check the end of the time window
        // (add possible time warp)
        routeTimeWarp += std::max(time - data.depot().twLate, 0);

        // Whole solution stats
        distance_ += routeDist;
        timeWarp_ += routeTimeWarp;

        if (static_cast<size_t>(routeLoad) > data.vehicleCapacity())
            excessLoad_ += routeLoad - data.vehicleCapacity();

        uncollected_ -= routePrize;
    }
}

size_t Individual::numRoutes() const { return numRoutes_; }

size_t Individual::size() const { return size_; }

Routes const &Individual::getRoutes() const { return routes_; }

std::vector<std::pair<Client, Client>> const &Individual::getNeighbours() const
{
    return neighbours;
}

bool Individual::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::hasTimeWarp() const { return timeWarp_ > 0; }

size_t Individual::distance() const { return distance_; }

size_t Individual::excessLoad() const { return excessLoad_; }

size_t Individual::uncollected() const { return uncollected_; }

size_t Individual::timeWarp() const { return timeWarp_; }

void Individual::makeNeighbours()
{
    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

bool Individual::operator==(Individual const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the neighbours are all equal.
    // clang-format off
    return distance_ == other.distance_
        && excessLoad_ == other.excessLoad_
        && timeWarp_ == other.timeWarp_
        && numRoutes_ == other.numRoutes_
        && neighbours == other.neighbours;
    // clang-format on
}

Individual::Individual(ProblemData const &data, XorShift128 &rng)
    : routes_(data.numVehicles()), neighbours(data.numClients() + 1, {0, 0})
{
    // Shuffle clients (to create random routes)
    auto clients = std::vector<int>(data.numClients());
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect.
    auto const numVehicles = data.numVehicles();
    auto const numClients = data.numClients();
    auto const perVehicle = std::max(numClients / numVehicles, size_t(1));
    auto const perRoute = perVehicle + (numClients % numVehicles != 0);

    for (size_t idx = 0; idx != numClients; ++idx)
        routes_[idx / perRoute].push_back(clients[idx]);

    makeNeighbours();
    evaluate(data);
}

Individual::Individual(ProblemData const &data, Routes routes)
    : routes_(std::move(routes)), neighbours(data.numClients() + 1, {0, 0})
{
    if (routes_.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    // Expand to at least numVehicles routes, where any newly inserted routes
    // will be empty.
    routes_.resize(data.numVehicles());

    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_.
    auto comp = [](auto &a, auto &b) { return !a.empty() && b.empty(); };
    std::stable_sort(routes_.begin(), routes_.end(), comp);

    makeNeighbours();
    evaluate(data);
}

std::ostream &operator<<(std::ostream &out, Individual const &indiv)
{
    auto const &routes = indiv.getRoutes();

    for (size_t rIdx = 0; rIdx != indiv.numRoutes(); ++rIdx)
    {
        out << "Route #" << rIdx + 1 << ":";  // route number
        for (int cIdx : routes[rIdx])
            out << " " << cIdx;  // client index
        out << '\n';
    }

    out << "Distance: " << indiv.distance() << '\n';
    return out;
}
