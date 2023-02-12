#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client const &ProblemData::depot() const { return client(0); }

Matrix<int> const &ProblemData::distanceMatrix() const { return dist_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

size_t ProblemData::vehicleCapacity() const { return vehicleCapacity_; }

ProblemData::ProblemData(std::vector<std::pair<int, int>> const &coords,
                         std::vector<int> const &demands,
                         size_t numVehicles,
                         size_t vehicleCap,
                         std::vector<std::pair<int, int>> const &timeWindows,
                         std::vector<int> const &servDurs,
                         std::vector<std::vector<int>> const &distMat,
                         std::vector<int> const &releases)
    : dist_(distMat),
      clients_(coords.size()),
      numClients_(static_cast<int>(coords.size()) - 1),
      numVehicles_(numVehicles),
      vehicleCapacity_(vehicleCap)
{
    for (size_t idx = 0; idx <= static_cast<size_t>(numClients_); ++idx)
        clients_[idx] = {coords[idx].first,
                         coords[idx].second,
                         servDurs[idx],
                         demands[idx],
                         timeWindows[idx].first,
                         timeWindows[idx].second,
                         releases[idx]};
}
