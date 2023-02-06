#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>
#include <vector>

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

void Individual::evaluateCompleteCost()
{
    // TODO simplify implementation
    nbRoutes = 0;
    distance = 0;
    capacityExcess = 0;
    timeWarp = 0;

    for (auto const &route : routes_)
    {
        if (route.empty())  // First empty route. All subsequent routes are
            break;          // empty as well.

        nbRoutes++;

        int lastRelease = 0;
        for (auto const idx : route)
            lastRelease = std::max(lastRelease, data->client(idx).releaseTime);

        int rDist = data->dist(0, route[0]);
        int rTimeWarp = 0;

        int load = data->client(route[0]).demand;
        int time = lastRelease + rDist;

        if (time < data->client(route[0]).twEarly)
            time = data->client(route[0]).twEarly;

        if (time > data->client(route[0]).twLate)
        {
            rTimeWarp += time - data->client(route[0]).twLate;
            time = data->client(route[0]).twLate;
        }

        for (size_t idx = 1; idx < route.size(); idx++)
        {
            // Sum the rDist, load, servDur and time associated with the vehicle
            // traveling from the depot to the next client
            rDist += data->dist(route[idx - 1], route[idx]);
            load += data->client(route[idx]).demand;

            time += data->client(route[idx - 1]).servDur
                    + data->dist(route[idx - 1], route[idx]);

            // Add possible waiting time
            if (time < data->client(route[idx]).twEarly)
                time = data->client(route[idx]).twEarly;

            // Add possible time warp
            if (time > data->client(route[idx]).twLate)
            {
                rTimeWarp += time - data->client(route[idx]).twLate;
                time = data->client(route[idx]).twLate;
            }
        }

        // For the last client, the successors is the depot. Also update the
        // rDist and time
        rDist += data->dist(route.back(), 0);
        time
            += data->client(route.back()).servDur + data->dist(route.back(), 0);

        // For the depot, we only need to check the end of the time window
        // (add possible time warp)
        rTimeWarp += std::max(time - data->depot().twLate, 0);

        // Whole solution stats
        distance += rDist;
        timeWarp += rTimeWarp;

        if (static_cast<size_t>(load) > data->vehicleCapacity())
            capacityExcess += load - data->vehicleCapacity();
    }
}

size_t Individual::cost() const
{
    auto const load = data->vehicleCapacity() + capacityExcess;
    auto const loadPenalty = penaltyManager->loadPenalty(load);
    auto const twPenalty = penaltyManager->twPenalty(timeWarp);

    return distance + loadPenalty + twPenalty;
}

size_t Individual::numRoutes() const { return nbRoutes; }

Routes const &Individual::getRoutes() const { return routes_; }

std::vector<std::pair<Client, Client>> const &Individual::getNeighbours() const
{
    return neighbours;
}

bool Individual::isFeasible() const
{
    return !hasExcessCapacity() && !hasTimeWarp();
}

bool Individual::hasExcessCapacity() const { return capacityExcess > 0; }

bool Individual::hasTimeWarp() const { return timeWarp > 0; }

void Individual::makeNeighbours()
{
    neighbours[0] = {0, 0};  // note that depot neighbours have no meaning

    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

Individual::Individual(ProblemData const &data,
                       PenaltyManager const &penaltyManager,
                       XorShift128 &rng)
    : data(&data),
      penaltyManager(&penaltyManager),
      routes_(data.numVehicles()),
      neighbours(data.numClients() + 1)
{
    // Shuffle clients (to create random routes)
    auto clients = std::vector<int>(data.numClients());
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect.
    auto const numVehicles = data.numVehicles();
    auto const perVehicle = std::max(data.numClients() / numVehicles, 1UL);
    auto const perRoute = perVehicle + (data.numClients() % numVehicles != 0);

    for (size_t idx = 0; idx != data.numClients(); ++idx)
        routes_[idx / perRoute].push_back(clients[idx]);

    makeNeighbours();
    evaluateCompleteCost();
}

Individual::Individual(ProblemData const &data,
                       PenaltyManager const &penaltyManager,
                       Routes routes)
    : data(&data),
      penaltyManager(&penaltyManager),
      routes_(std::move(routes)),
      neighbours(data.numClients() + 1)
{
    if (routes_.size() != static_cast<size_t>(data.numVehicles()))
    {
        auto const msg = "Number of routes does not match number of vehicles.";
        throw std::runtime_error(msg);
    }

    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_.
    auto comp = [](auto &a, auto &b) { return !a.empty() && b.empty(); };
    std::stable_sort(routes_.begin(), routes_.end(), comp);

    makeNeighbours();
    evaluateCompleteCost();
}

Individual::Individual(Individual const &other)  // copy fields from other
    : nbRoutes(other.nbRoutes),                  // individual
      distance(other.distance),
      capacityExcess(other.capacityExcess),
      timeWarp(other.timeWarp),
      data(other.data),
      penaltyManager(other.penaltyManager),
      routes_(other.routes_),
      neighbours(other.neighbours)
{
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

    out << "Cost: " << indiv.cost() << '\n';
    return out;
}
