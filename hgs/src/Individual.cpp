#include "Individual.h"
#include "Params.h"

#include <fstream>
#include <numeric>
#include <vector>

void Individual::evaluateCompleteCost()
{
    // Reset fields before evaluating them again below.
    nbRoutes = 0;
    distance = 0;
    capacityExcess = 0;
    timeWarp = 0;

    for (auto const &route : routes_)
    {
        if (route.empty())  // First empty route. All subsequent routes are
            break;          // empty as well

        nbRoutes++;

        int lastRelease = 0;
        for (auto const idx : route)
            lastRelease
                = std::max(lastRelease, params->clients[idx].releaseTime);

        int rDist = params->dist(0, route[0]);
        int rTimeWarp = 0;

        int load = params->clients[route[0]].demand;
        int time = lastRelease + rDist;

        if (time < params->clients[route[0]].twEarly)
            time = params->clients[route[0]].twEarly;

        if (time > params->clients[route[0]].twLate)
        {
            rTimeWarp += time - params->clients[route[0]].twLate;
            time = params->clients[route[0]].twLate;
        }

        for (size_t idx = 1; idx < route.size(); idx++)
        {
            // Sum the rDist, load, servDur and time associated with the vehicle
            // traveling from the depot to the next client
            rDist += params->dist(route[idx - 1], route[idx]);
            load += params->clients[route[idx]].demand;

            time += params->clients[route[idx - 1]].servDur
                    + params->dist(route[idx - 1], route[idx]);

            // Add possible waiting time
            if (time < params->clients[route[idx]].twEarly)
                time = params->clients[route[idx]].twEarly;

            // Add possible time warp
            if (time > params->clients[route[idx]].twLate)
            {
                rTimeWarp += time - params->clients[route[idx]].twLate;
                time = params->clients[route[idx]].twLate;
            }
        }

        // For the last client, the successors is the depot. Also update the
        // rDist and time
        rDist += params->dist(route.back(), 0);
        time += params->clients[route.back()].servDur
                + params->dist(route.back(), 0);

        // For the depot, we only need to check the end of the time window
        // (add possible time warp)
        rTimeWarp += std::max(time - params->clients[0].twLate, 0);

        // Whole solution stats
        distance += rDist;
        timeWarp += rTimeWarp;
        capacityExcess += std::max(load - params->vehicleCapacity, 0);
    }
}

int Individual::brokenPairsDistance(Individual const *other) const
{
    int dist = 0;

    for (int j = 1; j <= params->nbClients; j++)
    {
        auto const [tPred, tSucc] = this->neighbours[j];
        auto const [oPred, oSucc] = other->neighbours[j];

        // Increase the difference if the successor of j in this individual is
        // not directly linked to j in other
        dist += tSucc != oSucc && tSucc != oPred;

        // Increase the difference if the predecessor of j in this individual is
        // not directly linked to j in other
        dist += tPred == 0 && oPred != 0 && oSucc != 0;
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

    auto maxSize = std::min(params->config.nbClose, indivsByProximity.size());
    auto start = indivsByProximity.begin();
    int result = 0;

    for (auto it = start; it != start + maxSize; ++it)
        result += it->first;

    // Normalise broken pairs distance by # of clients and close neighbours
    auto const numClose = static_cast<double>(maxSize);
    return result / (params->nbClients * numClose);
}

void Individual::exportCVRPLibFormat(std::string const &path, double time) const
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

Individual::Individual(Params const *params, XorShift128 *rng)
    : params(params),
      routes_(params->nbVehicles),
      neighbours(params->nbClients + 1)
{
    auto const nbClients = params->nbClients;
    auto const nbVehicles = params->nbVehicles;

    // Sort clients randomly
    auto clients = std::vector<int>(nbClients);
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), *rng);

    // Distribute clients evenly over the routes
    auto const clientsPerRoute
        = std::max(nbClients / nbVehicles, 1) + (nbClients % nbVehicles != 0);

    for (auto idx = 0; idx != params->nbClients; ++idx)
    {
        auto const client = clients[idx];
        routes_[idx / clientsPerRoute].push_back(client);
    }

    makeNeighbours();
    evaluateCompleteCost();
}

Individual::Individual(Params const *params, Routes routes)
    : params(params),
      routes_(std::move(routes)),
      neighbours(params->nbClients + 1)
{
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
      params(other.params),                      // population.
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
