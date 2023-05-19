#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>
#include <sstream>

using Client = int;
using Visits = std::vector<Client>;
using Routes = std::vector<Individual::Route>;

void Individual::evaluate(ProblemData const &data)
{
    size_t allPrizes = 0;
    for (size_t client = 1; client <= data.numClients(); ++client)
        allPrizes += data.client(client).prize;

    for (auto const &route : routes_)
    {
        if (route.empty())  // First empty route. All subsequent routes are
            break;          // empty as well.

        // Whole solution statistics.
        numRoutes_++;
        numClients_ += route.size();
        prizes_ += route.prizes();
        distance_ += route.distance();
        timeWarp_ += route.timeWarp();
        excessLoad_ += route.excessLoad();
    }

    uncollectedPrizes_ = allPrizes - prizes_;
}

size_t Individual::numRoutes() const { return numRoutes_; }

size_t Individual::numClients() const { return numClients_; }

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

size_t Individual::prizes() const { return prizes_; }

size_t Individual::uncollectedPrizes() const { return uncollectedPrizes_; }

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

    std::vector<std::vector<Client>> routes(data.numVehicles());
    for (size_t idx = 0; idx != numClients; ++idx)
        routes[idx / perRoute].push_back(clients[idx]);

    for (size_t idx = 0; idx != routes.size(); ++idx)
        routes_[idx] = Route(data, routes[idx]);

    makeNeighbours();
    evaluate(data);
}

Individual::Individual(ProblemData const &data,
                       std::vector<std::vector<Client>> const &routes)
    : routes_(data.numVehicles()), neighbours(data.numClients() + 1, {0, 0})
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

Individual::Route::Route(ProblemData const &data, Visits const visits)
    : visits_(std::move(visits))
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
    excessLoad_ = data.vehicleCapacity() < demand_
                      ? demand_ - data.vehicleCapacity()
                      : 0;
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

bool Individual::Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::Route::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::Route::hasTimeWarp() const { return timeWarp_ > 0; }

std::ostream &operator<<(std::ostream &out, Individual const &indiv)
{
    auto const &routes = indiv.getRoutes();

    for (size_t idx = 0; idx != indiv.numRoutes(); ++idx)
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
