#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>

using Client = int;
using Routes = std::vector<Individual::Route>;

void Individual::evaluate(ProblemData const &data)
{
    for (auto const &route : routes_)
    {
        if (route.empty())
            break;

        // Whole solution statistics.
        numRoutes_++;
        distance_ += route.distance;
        timeWarp_ += route.timeWarp;

        if (static_cast<size_t>(route.demand) > data.vehicleCapacity())
            excessLoad_ += route.demand - data.vehicleCapacity();
    }
}

size_t Individual::numRoutes() const { return numRoutes_; }

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

size_t Individual::timeWarp() const { return timeWarp_; }

void Individual::makeNeighbours()
{
    neighbours[0] = {0, 0};  // note that depot neighbours have no meaning

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
    : routes_(data.numVehicles()), neighbours(data.numClients() + 1)
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

    std::vector<std::vector<Client>> routes(data.numVehicles());

    for (size_t idx = 0; idx != numClients; ++idx)
        routes[idx / perRoute].push_back(clients[idx]);

    for (size_t idx = 0; idx != routes.size(); ++idx)
        routes_[idx] = Route(data, routes[idx]);

    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_.
    auto comp = [](auto &a, auto &b) { return !a.empty() && b.empty(); };
    std::stable_sort(routes_.begin(), routes_.end(), comp);

    makeNeighbours();
    evaluate(data);
}

Individual::Individual(ProblemData const &data,
                       std::vector<std::vector<Client>> routes)
    : routes_(data.numVehicles()), neighbours(data.numClients() + 1)
{
    if (routes.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    for (size_t idx = 0; idx != routes.size(); ++idx)
        routes_[idx] = Route(data, routes[idx]);

    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_.
    auto comp = [](auto &a, auto &b) { return !a.empty() && b.empty(); };
    std::stable_sort(routes_.begin(), routes_.end(), comp);

    makeNeighbours();
    evaluate(data);
}

Individual::Individual(ProblemData const &data, Routes routes)
    : routes_(routes), neighbours(data.numClients() + 1)
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
        out << "Route #" << rIdx + 1 << ":" << routes[rIdx] << '\n';

    out << "Distance: " << indiv.distance() << '\n';
    return out;
}

std::ostream &operator<<(std::ostream &out, Individual::Route const &route)
{
    for (Client const client : route)
        out << client << ' ';
    return out;
}
