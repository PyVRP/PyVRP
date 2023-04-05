#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client const &ProblemData::depot() const { return client(0); }

Matrix<int> const &ProblemData::distanceMatrix() const { return dist_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::maxNumRoutes() const { return numAvailableRoutes_; }

ProblemData::ProblemData(std::vector<std::pair<int, int>> const &coords,
                         std::vector<int> const &demands,
                         std::vector<size_t> const &vehicleCapacities,
                         std::vector<std::pair<int, int>> const &timeWindows,
                         std::vector<int> const &servDurs,
                         std::vector<std::vector<int>> const &distMat)
    : dist_(distMat),
      clients_(coords.size()),
      routes_(vehicleCapacities.size()),
      routeTypes_(vehicleCapacities.size()),
      numClients_(coords.size() - 1),
      numAvailableRoutes_(vehicleCapacities.size())
{
    // TODO argument checks

    for (size_t idx = 0; idx <= static_cast<size_t>(numClients_); ++idx)
        clients_[idx] = {coords[idx].first,
                         coords[idx].second,
                         servDurs[idx],
                         demands[idx],
                         timeWindows[idx].first,
                         timeWindows[idx].second};

    size_t routeType = 0;
    for (size_t idx = 0; idx < static_cast<size_t>(numAvailableRoutes_); ++idx)
    {
        routes_[idx] = {vehicleCapacities[idx]};
        if (idx > 0 && routes_[idx] != routes_[idx - 1])
            routeType++;
        routeTypes_[idx] = routeType;
    }
}
