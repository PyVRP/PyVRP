#include "ProblemData.h"

#include <numeric>

using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Matrix;
using pyvrp::ProblemData;

ProblemData::Client::Client(Coordinate x,
                            Coordinate y,
                            Load demand,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Duration releaseTime,
                            Cost prize,
                            bool required)
    : x(x),
      y(y),
      demand(demand),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      releaseTime(releaseTime),
      prize(prize),
      required(required)
{
    if (demand < 0)
        throw std::invalid_argument("demand must be >= 0");

    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late");

    if (prize < 0)
        throw std::invalid_argument("prize must be >= 0");
}

std::vector<ProblemData::Client> const &ProblemData::clients() const
{
    return clients_;
}

std::vector<ProblemData::Client> const &ProblemData::depots() const
{
    return depots_;
}

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numDepots() const { return numDepots_; }

size_t ProblemData::dimension() const { return numDepots_ + numClients_; }

size_t ProblemData::numVehicleTypes() const { return numVehicleTypes_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         std::vector<Client> const &depots,
                         std::vector<VehicleType> const &vehicleTypes,
                         Matrix<Distance> distMat,
                         Matrix<Duration> durMat)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(clients),
      depots_(depots),
      vehicleTypes_(vehicleTypes),
      numClients_(clients.size()),
      numDepots_(depots.size()),
      numVehicleTypes_(vehicleTypes.size()),
      numVehicles_(std::accumulate(vehicleTypes.begin(),
                                   vehicleTypes.end(),
                                   0,
                                   [](auto sum, VehicleType const &type) {
                                       return sum + type.numAvailable;
                                   }))
{
    if (depots.size() != 1)
        throw std::invalid_argument("Expected a single depot!");

    if (dist_.numRows() != dimension() || dist_.numCols() != dimension())
        throw std::invalid_argument("Distance matrix shape does not match the "
                                    "problem dimension.");

    if (dur_.numRows() != dimension() || dur_.numCols() != dimension())
        throw std::invalid_argument("Duration matrix shape does not match the "
                                    "problem dimension.");

    auto const &depot = depots_[0];

    if (depot.demand != 0)
        throw std::invalid_argument("Depot demand must be 0.");

    if (depot.serviceDuration != 0)
        throw std::invalid_argument("Depot service duration must be 0.");

    if (depot.releaseTime != 0)
        throw std::invalid_argument("Depot release time must be 0.");

    for (auto &client : clients_)
    {
        centroid_.first += static_cast<double>(client.x) / numClients();
        centroid_.second += static_cast<double>(client.y) / numClients();
    }
}
