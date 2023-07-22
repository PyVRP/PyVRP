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

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

Matrix<Distance> const &ProblemData::distanceMatrix() const { return dist_; }

Matrix<Duration> const &ProblemData::durationMatrix() const { return dur_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicleTypes() const { return numVehicleTypes_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         std::vector<VehicleType> const &vehicleTypes,
                         Matrix<Distance> distMat,
                         Matrix<Duration> durMat)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(clients),
      vehicleTypes_(vehicleTypes),
      numClients_(std::max<size_t>(clients.size(), 1) - 1),
      numVehicleTypes_(vehicleTypes.size()),
      numVehicles_(std::accumulate(vehicleTypes.begin(),
                                   vehicleTypes.end(),
                                   0,
                                   [](auto sum, VehicleType const &type) {
                                       return sum + type.numAvailable;
                                   }))
{
    for (size_t idx = 1; idx <= numClients(); ++idx)
    {
        centroid_.first += static_cast<double>(clients[idx].x) / numClients();
        centroid_.second += static_cast<double>(clients[idx].y) / numClients();
    }
}
