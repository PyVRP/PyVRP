#include "Individual.h"
#include "ProblemData.h"

#include <fstream>
#include <numeric>

using Client = int;
using Plan = std::vector<int>;
using Routes = std::vector<Individual::Route>;

void Individual::evaluate(ProblemData const &data)
{
    for (auto const &route : routes_)
    {
        if (route.empty())
            break;

        // Whole solution statistics.
        numRoutes_++;
        distance_ += route.distance();
        timeWarp_ += route.timeWarp();
        excessLoad_ += route.excessLoad();
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

Individual::Route::Route(ProblemData const &data, Plan const &plan)
    : plan_(plan)
{
    if (plan.empty())
        return;

    int time = data.depot().twEarly;
    int prevClient = 0;

    for (size_t idx = 0; idx != size(); ++idx)
    {
        distance_ += data.dist(prevClient, plan[idx]);
        duration_ += data.duration(prevClient, plan[idx]);
        demand_ += data.client(plan[idx]).demand;

        time += data.client(prevClient).serviceDuration
                + data.duration(prevClient, plan[idx]);

        if (time < data.client(plan[idx]).twEarly)  // add wait duration
        {
            wait_ += data.client(plan[idx]).twEarly - time;
            time = data.client(plan[idx]).twEarly;
        }

        if (time > data.client(plan[idx]).twLate)  // add time warp
        {
            timeWarp_ += time - data.client(plan[idx]).twLate;
            time = data.client(plan[idx]).twLate;
        }

        prevClient = plan[idx];
    }

    // Last client has depot as successor.
    distance_ += data.dist(plan.back(), 0);
    duration_ += data.duration(plan.back(), 0);
    time += data.client(plan.back()).serviceDuration
            + data.duration(plan.back(), 0);

    excessLoad_ = data.vehicleCapacity() < demand_
                      ? demand_ - data.vehicleCapacity()
                      : 0;

    // For the depot we only need to check the end of the time window.
    timeWarp_ += std::max(time - data.depot().twLate, 0);
}

bool Individual::Route::empty() const { return plan_.empty(); }

size_t Individual::Route::size() const { return plan_.size(); }

Client Individual::Route::operator[](size_t idx) const { return plan_[idx]; }

Plan::const_iterator Individual::Route::begin() const { return plan_.cbegin(); }

Plan::const_iterator Individual::Route::end() const { return plan_.cend(); }

Plan::const_iterator Individual::Route::cbegin() const
{
    return plan_.cbegin();
}

Plan::const_iterator Individual::Route::cend() const { return plan_.cend(); }

std::vector<Client> const &Individual::Route::plan() const { return plan_; }

size_t Individual::Route::distance() const { return distance_; }

size_t Individual::Route::demand() const { return demand_; }

size_t Individual::Route::excessLoad() const { return excessLoad_; }

size_t Individual::Route::duration() const { return duration_; }

size_t Individual::Route::service() const { return service_; }

size_t Individual::Route::timeWarp() const { return timeWarp_; }

size_t Individual::Route::wait() const { return wait_; }

bool Individual::Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Individual::Route::hasExcessLoad() const { return excessLoad_ > 0; }

bool Individual::Route::hasTimeWarp() const { return timeWarp_ > 0; }

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
