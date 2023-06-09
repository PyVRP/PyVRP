#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>
#include <sstream>

using Client = int;
using Visits = std::vector<Client>;
using Routes = std::vector<Individual::Route>;
using RouteType = int;

void Individual::evaluate(ProblemData const &data)
{
    size_t allPrizes = 0;
    for (size_t client = 1; client <= data.numClients(); ++client)
        allPrizes += data.client(client).prize;

    for (size_t idx = 0; idx < data.maxNumRoutes(); idx++)
    {
        auto const &route = routes_[idx];
        if (route.empty())
            continue;

        // Whole solution statistics.
        numNonEmptyRoutes_++;
        numClients_ += route.size();
        prizes_ += route.prizes();
        distance_ += route.distance();
        timeWarp_ += route.timeWarp();
        excessLoad_ += route.excessLoad();
    }

    uncollectedPrizes_ = allPrizes - prizes_;
}

size_t Individual::numNonEmptyRoutes() const { return numNonEmptyRoutes_; }

size_t Individual::numClients() const { return numClients_; }

Routes const &Individual::getRoutes() const { return routes_; }

std::vector<std::pair<Client, Client>> const &Individual::getNeighbours() const
{
    return neighbours;
}

std::vector<RouteType> const &Individual::getAssignments() const
{
    return assignedRouteTypes;
}

bool Individual::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::hasTimeWarp() const { return timeWarp_ > 0; }

size_t Individual::distance() const { return distance_; }

size_t Individual::excessLoad() const { return excessLoad_; }

size_t Individual::prizes() const { return prizes_; }

size_t Individual::uncollectedPrizes() const { return uncollectedPrizes_; }

size_t Individual::timeWarp() const { return timeWarp_; }

void Individual::makeNeighbours(ProblemData const &data)
{
    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

void Individual::makeAssignedRouteTypes(ProblemData const &data)
{
    assignedRouteTypes[0] = -1;  // unassigned

    for (size_t rIdx = 0; rIdx != data.maxNumRoutes(); ++rIdx)
    {
        auto const route = routes_[rIdx];
        for (size_t idx = 0; idx != route.size(); ++idx)
            assignedRouteTypes[route[idx]] = data.routeType(rIdx);
    }
}

bool Individual::operator==(Individual const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the neighbours are all equal.
    // Only when that is also the case, we check if the assigned vehicle types
    // (capacities) are equal for the heterogeneous case.

    // clang-format off
    return distance_ == other.distance_
        && excessLoad_ == other.excessLoad_
        && timeWarp_ == other.timeWarp_
        && numNonEmptyRoutes_ == other.numNonEmptyRoutes_
        && neighbours == other.neighbours
        && assignedRouteTypes == other.assignedRouteTypes;
    // clang-format on
}

Individual::Individual(ProblemData const &data, XorShift128 &rng)
    : routes_(data.maxNumRoutes()),
      neighbours(data.numClients() + 1, {0, 0}),
      assignedRouteTypes(data.numClients() + 1)
{
    // Shuffle clients (to create random routes)
    auto clients = std::vector<int>(data.numClients());
    std::iota(clients.begin(), clients.end(), 1);
    std::shuffle(clients.begin(), clients.end(), rng);

    // Distribute clients evenly over the routes: the total number of clients
    // per vehicle, with an adjustment in case the division is not perfect.
    auto const maxNumRoutes = data.maxNumRoutes();
    auto const numClients = data.numClients();
    auto const perRouteFloor = std::max(numClients / maxNumRoutes, size_t(1));
    auto const perRoute = perRouteFloor + (numClients % maxNumRoutes != 0);

    std::vector<std::vector<Client>> routes(data.maxNumRoutes());
    for (size_t idx = 0; idx != numClients; ++idx)
        routes[idx / perRoute].push_back(clients[idx]);

    for (size_t idx = 0; idx != routes.size(); ++idx)
        routes_[idx] = Route(data, routes[idx], idx);

    makeNeighbours(data);
    makeAssignedRouteTypes(data);
    evaluate(data);
}

Individual::Individual(ProblemData const &data,
                       std::vector<std::vector<Client>> const &routes)
    : routes_(data.maxNumRoutes()),
      neighbours(data.numClients() + 1, {0, 0}),
      assignedRouteTypes(data.numClients() + 1)
{
    if (routes.size() > data.maxNumRoutes())
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

    for (size_t idx = 0; idx != routes_.size(); ++idx)
        routes_[idx]
            = Route(data, idx < routes.size() ? routes[idx] : Visits(), idx);

    // We sort routes by route types. Within routes of the same type
    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_ for each
    // route type.
    auto comp = [&data](auto &a, auto &b) {
        auto const typeA = a.typeIdx();
        auto const typeB = b.typeIdx();
        // If same type, empty vehicles first
        return typeA == typeB ? !a.empty() && b.empty() : typeA < typeB;
    };
    std::stable_sort(routes_.begin(), routes_.end(), comp);

    makeNeighbours(data);
    makeAssignedRouteTypes(data);
    evaluate(data);
}

Individual::Route::Route(ProblemData const &data,
                         Visits const visits,
                         size_t const rIdx)
    : visits_(std::move(visits)), typeIdx_(data.routeType(rIdx))
{
    if (visits_.empty())
        return;

    int time = data.depot().twEarly;
    int prevClient = 0;

    for (size_t idx = 0; idx != size(); ++idx)
    {
        auto const &clientData = data.client(visits_[idx]);

        distance_ += data.dist(prevClient, visits_[idx]);
        duration_ += data.duration(prevClient, visits_[idx]);
        demand_ += clientData.demand;
        service_ += clientData.serviceDuration;
        prizes_ += clientData.prize;

        time += data.client(prevClient).serviceDuration
                + data.duration(prevClient, visits_[idx]);

        if (time < clientData.twEarly)  // add wait duration
        {
            wait_ += clientData.twEarly - time;
            time = clientData.twEarly;
        }

        if (time > clientData.twLate)  // add time warp
        {
            timeWarp_ += time - clientData.twLate;
            time = clientData.twLate;
        }

        prevClient = visits_[idx];
    }

    Client const last = visits_.back();  // last client has depot as successor
    distance_ += data.dist(last, 0);
    duration_ += data.duration(last, 0);
    time += data.client(last).serviceDuration + data.duration(last, 0);
    timeWarp_ += std::max(time - data.depot().twLate, 0);  // depot closing tw
    auto const capacity = data.routeData(rIdx).capacity;
    excessLoad_ = capacity < demand_ ? demand_ - capacity : 0;
}

bool Individual::Route::empty() const { return visits_.empty(); }

size_t Individual::Route::size() const { return visits_.size(); }

Client Individual::Route::operator[](size_t idx) const { return visits_[idx]; }

Visits::const_iterator Individual::Route::begin() const
{
    return visits_.cbegin();
}

Visits::const_iterator Individual::Route::end() const { return visits_.cend(); }

Visits::const_iterator Individual::Route::cbegin() const
{
    return visits_.cbegin();
}

Visits::const_iterator Individual::Route::cend() const
{
    return visits_.cend();
}

Visits const &Individual::Route::visits() const { return visits_; }

size_t Individual::Route::distance() const { return distance_; }

size_t Individual::Route::demand() const { return demand_; }

size_t Individual::Route::excessLoad() const { return excessLoad_; }

size_t Individual::Route::duration() const { return duration_; }

size_t Individual::Route::serviceDuration() const { return service_; }

size_t Individual::Route::timeWarp() const { return timeWarp_; }

size_t Individual::Route::waitDuration() const { return wait_; }

size_t Individual::Route::prizes() const { return prizes_; }

size_t Individual::Route::typeIdx() const { return typeIdx_; }

bool Individual::Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::Route::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::Route::hasTimeWarp() const { return timeWarp_ > 0; }

std::ostream &operator<<(std::ostream &out, Individual const &indiv)
{
    // Since non-empty routes are guaranteed to come before empty routes
    // this will print consecutive route numbers for homogeneous problem
    // instances, but there may be gaps in the route indices corresponding
    // to different vehicle capacities.
    auto const &routes = indiv.getRoutes();

    for (size_t idx = 0; idx != routes.size(); ++idx)
    {
        if (routes[idx].empty())
            continue;
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';
    }

    out << "Distance: " << indiv.distance() << '\n';
    out << "Prizes: " << indiv.prizes() << '\n';
    return out;
}

std::ostream &operator<<(std::ostream &out, Individual::Route const &route)
{
    for (Client const client : route)
        out << client << ' ';
    return out;
}
