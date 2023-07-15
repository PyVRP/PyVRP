#include "Solution.h"
#include "ProblemData.h"

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
        excessWeight_ += route.excessWeight();
        excessVolume_ += route.excessVolume();
        excessSalvage_ += route.excessSalvage();
        // excessSalvageSequence_ += route.excessSalvageSequence();
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

bool Solution::isFeasible() const { return !hasExcessWeight() && !hasExcessVolume() && !hasExcessSalvage() && !hasTimeWarp(); }
// bool Solution::isFeasible() const { return !hasExcessWeight() && !hasExcessVolume() && !hasExcessSalvage() && !hasExcessSalvageSequence() && !hasTimeWarp(); }

bool Solution::hasExcessWeight() const { return excessWeight_ > 0; }
bool Solution::hasExcessVolume() const { return excessVolume_ > 0; }
bool Solution::hasExcessSalvage() const { return excessSalvage_ > 0; }
// bool Solution::hasExcessSalvageSequence() const { return excessSalvageSequence_ > 0; }
// bool Solution::hasSalvageBeforeDelivery() const { return salvageBeforeDelivery_; }
bool Solution::hasTimeWarp() const { return timeWarp_ > 0; }

Distance Solution::distance() const { return distance_; }

Load Solution::excessWeight() const { return excessWeight_; }
Load Solution::excessVolume() const { return excessVolume_; }
Salvage Solution::excessSalvage() const { return excessSalvage_; }
// Salvage Solution::excessSalvageSequence() const { return excessSalvageSequence_; }

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
        && excessWeight_ == other.excessWeight_
        && excessVolume_ == other.excessVolume_
        && excessSalvage_ == other.excessSalvage_
//        && excessSalvageSequence_ == other.excessSalvageSequence_
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

    Duration time = data.depot().twEarly;
    int prevClient = 0;

//    bool foundSalvage = false;
//    bool foundDelivery = false;
//    bool isSalvage = false;
//    bool isDelivery = false;
//    Salvage salvageCount = 0;
//    Salvage salvageSequenceViolations = 0;

//    std::cout << "###### Enter Solution:Route" << std::endl << std::endl;
    for (size_t idx = 0; idx != size(); ++idx)
    {
        auto const &clientData = data.client(visits_[idx]);

        distance_ += data.dist(prevClient, visits_[idx]);
        duration_ += data.duration(prevClient, visits_[idx]);
        demandWeight_ += clientData.demandWeight;
        demandVolume_ += clientData.demandVolume;
        demandSalvage_ += clientData.demandSalvage;
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

//        if (clientData.demandSalvage) 
//        {
//            isSalvage = true;
//        }
//
//        if (clientData.demandWeight || clientData.demandVolume)
//        {
//            isDelivery = true;
//        }
//
//        if (isDelivery && foundSalvage)
//        {
//            salvageBeforeDelivery_ = true;
//            salvageSequenceViolations += 1;
//        }
//
//        salvageCount = clientData.demandSalvage ? salvageCount + Salvage(1) : salvageCount;
//
//        if (isSalvage)
//        {
//            if (!foundSalvage)
//                foundSalvage = true;
//        }
//
//        if (clientData.demandSalvage && foundDelivery) 
//        {
//            salvageBeforeDelivery_ = true;
//        }
//
//        if (!clientData.demandSalvage) 
//        {
//            foundDelivery = true;
//        }

//        std::cout << "Route Constructor Node: " << idx
//            << " salvageCount: " << salvageCount
//          << " clientData.demandVolume: " << clientData.demandVolume
//          << " clientData.demandSalvage: " << clientData.demandSalvage
//            << " foundSalvage: " << foundSalvage
//            << " SalvageBefore: " << salvageBeforeDelivery_
//          << std::endl;
        prevClient = visits_[idx];
    }

    Client const last = visits_.back();  // last client has depot as successor
    distance_ += data.dist(last, 0);
    duration_ += data.duration(last, 0);

    time += data.client(last).serviceDuration + data.duration(last, 0);
    timeWarp_ += std::max<Duration>(time - data.depot().twLate, 0);

    excessWeight_ = data.weightCapacity() < demandWeight_
                      ? demandWeight_ - data.weightCapacity()
                      : 0;

    excessVolume_ = data.volumeCapacity() < demandVolume_
                      ? demandVolume_ - data.volumeCapacity()
                      : 0;

    excessSalvage_ = data.salvageCapacity() < demandSalvage_
                      ? demandSalvage_ - data.salvageCapacity()
                      : 0;

//    excessSalvageSequence_ = salvageBeforeDelivery_
//                      // ? Salvage(1)
//                      ? salvageSequenceViolations
//                      : 0;

    // Debug prints
//    std::cout << "In Solution:Route: excessWeight_: " << excessWeight_
//              << ", excessVolume_: " << excessVolume_
//              << ", excessSalvage_: " << excessSalvage_ << std::endl;
//              << ", excessSalvageSequence_: " << excessSalvageSequence_ << std::endl;
//    std::cout << "###### Exit Solution:Route" << std::endl << std::endl;
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

Load Solution::Route::demandWeight() const { return demandWeight_; }

Load Solution::Route::demandVolume() const { return demandVolume_; }

Salvage Solution::Route::demandSalvage() const { return demandSalvage_; }

Load Solution::Route::excessWeight() const { return excessWeight_; }

Load Solution::Route::excessVolume() const { return excessVolume_; }

Salvage Solution::Route::excessSalvage() const { return excessSalvage_; }

// Salvage Solution::Route::excessSalvageSequence() const { return excessSalvageSequence_; }

Duration Solution::Route::duration() const { return duration_; }

Duration Solution::Route::serviceDuration() const { return service_; }

Duration Solution::Route::timeWarp() const { return timeWarp_; }

Duration Solution::Route::waitDuration() const { return wait_; }

Cost Solution::Route::prizes() const { return prizes_; }

std::pair<double, double> const &Solution::Route::centroid() const
{
    return centroid_;
}

bool Solution::Route::isFeasible() const
{
    return !hasExcessWeight() && !hasExcessVolume() && !hasExcessSalvage() && !hasTimeWarp();
//    return !hasExcessWeight() && !hasExcessVolume() && !hasExcessSalvage() && !hasTimeWarp() && !hasExcessSalvageSequence();
}

bool Solution::Route::hasExcessWeight() const { return excessWeight_ > 0; }

bool Solution::Route::hasExcessVolume() const { return excessVolume_ > 0; }

bool Solution::Route::hasExcessSalvage() const { return excessSalvage_ > 0; }

// bool Solution::Route::hasExcessSalvageSequence() const { return excessSalvageSequence_ > 0; }

// bool Solution::Route::hasSalvageBeforeDelivery() const { return salvageBeforeDelivery_; }

bool Solution::Route::hasTimeWarp() const { return timeWarp_ > 0; }

// std::ostream &operator<<(std::ostream &out, Solution const &sol)
// {
//     auto const &routes = sol.getRoutes();
// 
//     for (size_t idx = 0; idx != routes.size(); ++idx)
//         out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';
// 
//     return out;
// }

std::ostream &operator<<(std::ostream &out, Solution const &sol)
{
    auto const &routes = sol.getRoutes();

    for (size_t idx = 0; idx != routes.size(); ++idx)
    {
        out << "Route #" << idx + 1 << ": " << routes[idx] << '\n';

        if (routes[idx].hasExcessWeight())
            out << "Excess weight: " << routes[idx].excessWeight() << '\n';

        if (routes[idx].hasExcessVolume())
            out << "Excess volume: " << routes[idx].excessVolume() << '\n';

        if (routes[idx].hasExcessSalvage())
            out << "Excess salvage: " << routes[idx].excessSalvage() << '\n';

//        if (routes[idx].hasExcessSalvageSequence())
//            out << "Excess salvage sequence: " << routes[idx].excessSalvageSequence() << '\n';
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, Solution::Route const &route)
{
    for (Client const client : route)
        out << client << ' ';
    return out;
}
