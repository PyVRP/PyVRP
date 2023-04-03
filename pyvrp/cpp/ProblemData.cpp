#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client const &ProblemData::depot() const { return client(0); }

Matrix<distance_type> const &ProblemData::distanceMatrix() const
{
    return dist_;
}

Matrix<duration_type> const &ProblemData::durationMatrix() const
{
    return dist_;
}

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

size_t ProblemData::vehicleCapacity() const { return vehicleCapacity_; }

ProblemData::ProblemData(
    std::vector<std::pair<distance_type, distance_type>> const &coords,
    std::vector<int> const &demands,
    size_t numVehicles,
    size_t vehicleCap,
    std::vector<std::pair<duration_type, duration_type>> const &timeWindows,
    std::vector<duration_type> const &servDurs,
    std::vector<std::vector<distance_type>> const &distMat)
    : dist_(distMat),
      clients_(coords.size()),
      numClients_(static_cast<int>(coords.size()) - 1),
      numVehicles_(numVehicles),
      vehicleCapacity_(vehicleCap)
{
    // TODO argument checks

    for (size_t idx = 0; idx <= static_cast<size_t>(numClients_); ++idx)
        clients_[idx] = {coords[idx].first,
                         coords[idx].second,
                         demands[idx],
                         servDurs[idx],
                         timeWindows[idx].first,
                         timeWindows[idx].second};
}
