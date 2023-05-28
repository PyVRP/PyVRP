#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client::Client(int x,
                            int y,
                            int demand,
                            int serviceDuration,
                            int twEarly,
                            int twLate,
                            int prize,
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

Matrix<int> const &ProblemData::distanceMatrix() const { return dist_; }

Matrix<int> const &ProblemData::durationMatrix() const { return dur_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

size_t ProblemData::vehicleCapacity() const { return vehicleCapacity_; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         size_t numVehicles,
                         size_t vehicleCap,
                         std::vector<std::vector<int>> const &distMat,
                         std::vector<std::vector<int>> const &durMat)
    : dist_(distMat),
      dur_(durMat),
      clients_(clients),
      numClients_(std::max(clients.size(), static_cast<size_t>(1)) - 1),
      numVehicles_(numVehicles),
      vehicleCapacity_(vehicleCap)
{
}
