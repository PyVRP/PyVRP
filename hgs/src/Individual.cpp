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
            lastRelease = std::max(lastRelease, data->clients[idx].releaseTime);

        int rDist = data->dist(0, route[0]);
        int rTimeWarp = 0;

        int load = data->clients[route[0]].demand;
        int time = lastRelease + rDist;

        if (time < data->clients[route[0]].twEarly)
            time = data->clients[route[0]].twEarly;

        if (time > data->clients[route[0]].twLate)
        {
            rTimeWarp += time - data->clients[route[0]].twLate;
            time = data->clients[route[0]].twLate;
        }

        for (size_t idx = 1; idx < route.size(); idx++)
        {
            // Sum the rDist, load, servDur and time associated with the vehicle
            // traveling from the depot to the next client
            rDist += data->dist(route[idx - 1], route[idx]);
            load += data->clients[route[idx]].demand;

            time += data->clients[route[idx - 1]].servDur
                    + data->dist(route[idx - 1], route[idx]);

            // Add possible waiting time
            if (time < data->clients[route[idx]].twEarly)
                time = data->clients[route[idx]].twEarly;

            // Add possible time warp
            if (time > data->clients[route[idx]].twLate)
            {
                rTimeWarp += time - data->clients[route[idx]].twLate;
                time = data->clients[route[idx]].twLate;
            }
        }

        // For the last client, the successors is the depot. Also update the
        // rDist and time
        rDist += data->dist(route.back(), 0);
        time += data->clients[route.back()].servDur
                + data->dist(route.back(), 0);

        // For the depot, we only need to check the end of the time window
        // (add possible time warp)
        rTimeWarp += std::max(time - data->clients[0].twLate, 0);

        // Whole solution stats
        distance += rDist;
        timeWarp += rTimeWarp;
        capacityExcess += std::max(load - data->vehicleCapacity, 0);
    }
}

size_t Individual::cost() const
{
    auto const load = data->vehicleCapacity + capacityExcess;
    auto const loadPenalty = data->pManager.loadPenalty(load);
    auto const twPenalty = data->pManager.twPenalty(timeWarp);

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

bool Individual::hasClone() const
{
    return !indivsByProximity.empty() && indivsByProximity.begin()->first == 0;
}

int Individual::brokenPairsDistance(Individual const *other) const
{
    int dist = 0;

    for (int j = 1; j <= data->nbClients; j++)
    {
        auto const [tPred, tSucc] = this->neighbours[j];
        auto const [oPred, oSucc] = other->neighbours[j];

        // The pair (j, tSucc) is broken in only one of two cases:
        // 1. tSucc does not precede or succeed j in the other solution;
        // 2. When our predecessor is the depot, and j in the other solution
        //    is not adjacent to a depot.
        dist += (tSucc != oSucc && tSucc != oPred)
                || (tPred == 0 && oPred != 0 && oSucc != 0);
    }

    return dist;
}

void Individual::registerNearbyIndividual(Individual *other)
{
    auto const dist = brokenPairsDistance(other);
    auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

    auto &oProx = other->indivsByProximity;
    auto place = std::lower_bound(oProx.begin(), oProx.end(), dist, cmp);
    oProx.emplace(place, dist, this);

    auto &tProx = this->indivsByProximity;
    place = std::lower_bound(tProx.begin(), tProx.end(), dist, cmp);
    tProx.emplace(place, dist, other);
}

double Individual::avgBrokenPairsDistanceClosest() const
{
    if (indivsByProximity.empty())
        return 0.;

    auto maxSize = std::min(data->config.nbClose, indivsByProximity.size());
    auto start = indivsByProximity.begin();
    int result = 0;

    for (auto it = start; it != start + maxSize; ++it)
        result += it->first;

    // Normalise broken pairs distance by # of clients and close neighbours
    auto const numClose = static_cast<double>(maxSize);
    return result / (data->nbClients * numClose);
}

void Individual::toFile(std::string const &path, double time) const
{
    std::ofstream out(path);

    if (!out)
        throw std::runtime_error("Could not open " + path);

    out << *this;
    out << "Time " << time << '\n';
}

void Individual::makeNeighbours()
{
    neighbours[0] = {0, 0};  // note that depot neighbours have no meaning

    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

Individual::Individual(ProblemData const &data, XorShift128 &rng)
    : data(&data), routes_(data.nbVehicles), neighbours(data.nbClients + 1)
{
    auto const nbClients = data.nbClients;
    auto const nbVehicles = data.nbVehicles;

    // Shuffle clients (to create random routes)
    auto clients = std::vector<int>(nbClients);
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect.
    auto const perVehicle = std::max(nbClients / nbVehicles, 1);
    auto const perRoute = perVehicle + (nbClients % nbVehicles != 0);

    for (auto idx = 0; idx != nbClients; ++idx)
        routes_[idx / perRoute].push_back(clients[idx]);

    makeNeighbours();
    evaluateCompleteCost();
}

Individual::Individual(ProblemData const &data, Routes routes)
    : data(&data), routes_(std::move(routes)), neighbours(data.nbClients + 1)
{
    if (routes_.size() != static_cast<size_t>(data.nbVehicles))
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

Individual::Individual(Individual const &other)  // copy relevant route and cost
    : nbRoutes(other.nbRoutes),                  // fields from other individual
      distance(other.distance),                  // - but *not* the proximity
      capacityExcess(other.capacityExcess),      // structure since the copy
      timeWarp(other.timeWarp),                  // is not yet part of the same
      data(other.data),                          // population.
      routes_(other.routes_),
      neighbours(other.neighbours)
{
}

Individual::~Individual()
{
    for (auto [dist, other] : indivsByProximity)
    {
        auto place = std::find(other->indivsByProximity.begin(),
                               other->indivsByProximity.end(),
                               std::make_pair(dist, this));

        if (place != other->indivsByProximity.end())
            other->indivsByProximity.erase(place);
    }
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

    out << "Cost " << indiv.cost() << '\n';
    return out;
}
