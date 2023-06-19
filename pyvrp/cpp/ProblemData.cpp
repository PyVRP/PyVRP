#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client::Client(Coordinate x,
                            Coordinate y,
                            Load demand,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Cost prize,
                            bool required)
    : x(x),
      y(y),
      demand(demand),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
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

ProblemData::Client const &ProblemData::depot() const { return client(0); }

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

Matrix<Distance> const &ProblemData::distanceMatrix() const { return dist_; }

Matrix<Duration> const &ProblemData::durationMatrix() const { return dur_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicleTypes() const { return numVehicleTypes_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

bool ProblemData::isHeterogeneous() const { return numVehicleTypes_ > 1; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         std::vector<VehicleType> const &vehicleTypes,
                         Matrix<Distance> const distMat,
                         Matrix<Duration> const durMat)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(clients),
      vehicleTypes_(vehicleTypes),
      numClients_(std::max<size_t>(clients.size(), 1) - 1),
      numVehicles_(std::accumulate(vehicleTypes.begin(),
                                   vehicleTypes.end(),
                                   0,
                                   [](int sum, VehicleType const &vehicleType) {
                                       return sum + vehicleType.numAvailable;
                                   })),
      numVehicleTypes_(vehicleTypes.size())
{
    for (size_t idx = 1; idx <= numClients(); ++idx)
    {
        centroid_.first += static_cast<double>(clients[idx].x) / numClients();
        centroid_.second += static_cast<double>(clients[idx].y) / numClients();
    }
}
