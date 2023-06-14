#include "Individual.h"
#include "ProblemData.h"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>
#include <vector>

using Client = int;
using Visits = std::vector<Client>;
using Routes = std::vector<Individual::Route>;
using RouteType = int;

void Individual::evaluate(ProblemData const &data)
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

size_t Individual::numRoutes() const { return routes_.size(); }

size_t Individual::numClients() const { return numClients_; }

Routes const &Individual::getRoutes() const { return routes_; }

std::vector<std::pair<Client, Client>> const &Individual::getNeighbours() const
{
    return neighbours;
}

std::vector<RouteType> const &Individual::getAssignedVehicleTypes() const
{
    return assignedVehicleTypes;
}

bool Individual::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::hasTimeWarp() const { return timeWarp_ > 0; }

Distance Individual::distance() const { return distance_; }

Load Individual::excessLoad() const { return excessLoad_; }

Cost Individual::prizes() const { return prizes_; }

Cost Individual::uncollectedPrizes() const { return uncollectedPrizes_; }

Duration Individual::timeWarp() const { return timeWarp_; }

void Individual::makeNeighbours()
{
    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            neighbours[route[idx]]
                = {idx == 0 ? 0 : route[idx - 1],                  // pred
                   idx == route.size() - 1 ? 0 : route[idx + 1]};  // succ
}

void Individual::makeAssignedVehicleTypes()
{
    assignedVehicleTypes.assign(assignedVehicleTypes.size(), -1);  // unassigned

    for (auto const &route : routes_)
        for (size_t idx = 0; idx != route.size(); ++idx)
            assignedVehicleTypes[route[idx]] = route.vehicleType();
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
        && routes_.size() == other.routes_.size()
        && neighbours == other.neighbours
        && assignedVehicleTypes == other.assignedVehicleTypes;
    // clang-format on
}

Individual::Individual(ProblemData const &data, XorShift128 &rng)
    : neighbours(data.numClients() + 1, {0, 0}),
      assignedVehicleTypes(data.numClients() + 1)
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
    for (size_t typeIdx = 0; typeIdx != data.numVehicleTypes(); ++typeIdx)
    {
        auto const numAvailable = data.vehicleType(typeIdx).numAvailable;
        for (size_t i = 0; i != numAvailable; ++i)
            if (routes_.size() < routes.size())
                routes_.emplace_back(data, routes[routes_.size()], typeIdx);
    }

    makeNeighbours();
    makeAssignedVehicleTypes();
    evaluate(data);
}

std::vector<Individual::Route>
Individual::transformRoutes(ProblemData const &data,
                            std::vector<std::vector<Client>> const &routes)
{
    std::vector<Route> transformedRoutes;
    std::transform(routes.begin(),
                   routes.end(),
                   std::back_inserter(transformedRoutes),
                   [&data](const std::vector<Client> &visits) {
                       return Route(data, visits, 0);
                   });
    return transformedRoutes;
}

Individual::Individual(ProblemData const &data,
                       std::vector<std::vector<Client>> const &routes)
    : Individual(data, transformRoutes(data, routes))
{
}

Individual::Individual(ProblemData const &data,
                       std::vector<Route> const &routes)
    : neighbours(data.numClients() + 1, {0, 0}),
      assignedVehicleTypes(data.numClients() + 1)
{
    if (routes.size() > data.numVehicles())
    {
        auto const msg = "Number of routes must not exceed number of vehicles.";
        throw std::runtime_error(msg);
    }

    std::vector<size_t> visits(data.numClients() + 1, 0);
    std::vector<size_t> used_vehicles(data.numVehicleTypes(), 0);
    for (auto const &route : routes)
    {
        if (route.empty())
        {
            std::ostringstream msg;
            msg << "Individual should not contain empty routes.";
            throw std::runtime_error(msg.str());
        }
        used_vehicles[route.vehicleType()]++;
        for (auto const client : route)
            visits[client]++;
    }

    for (size_t typeIdx = 0; typeIdx != data.numVehicleTypes(); typeIdx++)
        if (used_vehicles[typeIdx] > data.vehicleType(typeIdx).numAvailable)
        {
            std::ostringstream msg;
            msg << "Used more than " << data.vehicleType(typeIdx).numAvailable
                << " vehicles of type " << typeIdx << ".";
            throw std::runtime_error(msg.str());
        }

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
    for (auto const &route : routes)
        if (!route.empty())
            routes_.push_back(route);

    if (data.numVehicleTypes() > 0)
    {
        // We sort routes by vehicle types. Combined with a stable sort, this
        // ensures we keep the original sorting as much as possible.
        auto comp = [&data](auto &a, auto &b) {
            // If same type, empty vehicles first
            return a.vehicleType() < b.vehicleType();
        };
        std::stable_sort(routes_.begin(), routes_.end(), comp);
    }

    makeNeighbours();
    makeAssignedVehicleTypes();
    evaluate(data);
}

Individual::Route::Route(ProblemData const &data,
                         Visits const visits,
                         size_t const vehicleType)
    : visits_(std::move(visits)), centroid_({0, 0}), vehicleType_(vehicleType)
{
    if (visits_.empty())
        return;

    Duration time = data.depot().twEarly;
    int prevClient = 0;

    for (size_t idx = 0; idx != size(); ++idx)
    {
        auto const &clientData = data.client(visits_[idx]);

        distance_ += data.dist(prevClient, visits_[idx]);
        duration_ += data.duration(prevClient, visits_[idx]);
        demand_ += clientData.demand;
        service_ += clientData.serviceDuration;
        prizes_ += clientData.prize;

        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();

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
    timeWarp_ += std::max<Duration>(time - data.depot().twLate, 0);

    auto const capacity = data.vehicleType(vehicleType).capacity;
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

Distance Individual::Route::distance() const { return distance_; }

Load Individual::Route::demand() const { return demand_; }

Load Individual::Route::excessLoad() const { return excessLoad_; }

Duration Individual::Route::duration() const { return duration_; }

Duration Individual::Route::serviceDuration() const { return service_; }

Duration Individual::Route::timeWarp() const { return timeWarp_; }

Duration Individual::Route::waitDuration() const { return wait_; }

Cost Individual::Route::prizes() const { return prizes_; }

std::pair<double, double> const &Individual::Route::centroid() const
{
    return centroid_;
}

size_t Individual::Route::vehicleType() const { return vehicleType_; }

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
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';

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
